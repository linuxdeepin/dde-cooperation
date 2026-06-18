#!/usr/bin/env bash
# =============================================================================
# test-prj-running.sh
# dde-cooperation 全量单元测试 + 覆盖率报告一键生成
#
# 流程:
#   1. 配置全量构建 (覆盖率 flags + 各模块测试开关)
#   2. 编译整个项目 (让所有 src/*.cpp 插桩, 产生 .gcno)
#   3. lcov --initial 抓编译期基线 (未测代码以 0% 进入分母)
#   4. 清零计数器, 运行所有测试二进制 (带超时, 防止挂死)
#   5. lcov -c 抓运行时覆盖率
#   6. 合并 baseline + runtime
#   7. 过滤: 只保留 src/, 剔除 build 目录/生成文件(moc,autogen)/测试/第三方/依赖
#   8. genhtml 生成可浏览报告
#   9. 打印总览 + 按模块拆分
#
# 用法:
#   ./test-prj-running.sh             # 完整流程 (配置+编译+测试+报告)
#   ./test-prj-running.sh --report    # 跳过配置/编译, 仅重新跑测试+出报告
#
# 环境变量 (可覆盖默认值):
#   BUILD_DIR       构建目录         (默认 build-full)
#   TEST_TIMEOUT    单个测试超时秒数 (默认 120)
#
# 已知测试行为 (不影响报告生成, 仅为说明):
#   - netutil 的 SSL/TCP 连接测试(client->Connect())会无限阻塞, 已用 timeout
#     保护; 被杀则 netutil 覆盖率丢失。需要时单独跑: ./build-full/.../netutil_tests
#   - logging/netutil 有部分用例失败, slotipc/interface_test 会段错误 —— 这些是
#     测试自身问题, 报告仍会生成(覆盖已跑过的部分)。
#   - slotipc 测试有链接错误, 故默认 SLOTIPC_BUILD_TESTS=OFF。
# =============================================================================
set -uo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR:-build-full}"
COV_DIR="${BUILD_DIR}/cov"
TEST_TIMEOUT="${TEST_TIMEOUT:-120}"
declare -x QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"  # GUI 测试无头运行 (cooperation-core 等)
LCOV_RC=(--rc lcov_branch_coverage=0)

cd "$PROJECT_DIR"

# 颜色输出
C_GREEN='\033[0;32m'; C_CYAN='\033[0;36m'; C_YELLOW='\033[1;33m'; C_RESET='\033[0m'
step() { echo -e "\n${C_CYAN}========== $1 ==========${C_RESET}"; }
ok()   { echo -e "${C_GREEN}[OK]${C_RESET} $1"; }
warn() { echo -e "${C_YELLOW}[WARN]${C_RESET} $1"; }

REPORT_ONLY=0
[[ "${1:-}" == "--report" ]] && REPORT_ONLY=1

# -----------------------------------------------------------------------------
if [[ "$REPORT_ONLY" -eq 0 ]]; then
step "1/9  配置全量构建 (覆盖率 flags)"
rm -rf "$BUILD_DIR"
cmake -S . -B "$BUILD_DIR" \
  -DDOTEST=ON \
  -DBUILD_BASEKIT_TESTS=ON \
  -DBUILD_LOGGING_TESTS=ON \
  -DBUILD_NETUTIL_TESTS=ON \
  -DBUILD_TESTS=ON \
  -DSLOTIPC_BUILD_TESTS=OFF \
  -DENABLE_SLOTIPC=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage -g -O0" \
  -DCMAKE_EXE_LINKER_FLAGS="-fprofile-arcs -ftest-coverage"
[[ $? -eq 0 ]] && ok "配置完成" || { echo "配置失败"; exit 1; }

step "2/9  编译整个项目 (全部目标, 让 src/ 全量插桩)"
cmake --build "$BUILD_DIR" -j"$(nproc)"
[[ $? -eq 0 ]] && ok "编译完成" || warn "编译有错误 (部分目标可能未生成), 继续生成报告"
fi

mkdir -p "$COV_DIR"

# -----------------------------------------------------------------------------
step "3/9  抓编译期基线 lcov --initial (未测代码 = 0%, 进入分母)"
lcov --initial -c -d "$BUILD_DIR" -o "$COV_DIR/baseline.info" "${LCOV_RC[@]}" -q 2>/dev/null
ok "baseline 文件数: $(grep -c '^SF:' "$COV_DIR/baseline.info")"

# -----------------------------------------------------------------------------
step "4/9  清零覆盖率计数器并运行所有测试二进制"
find "$BUILD_DIR" -name '*.gcda' -delete 2>/dev/null

mapfile -t TEST_BINS < <(find "$BUILD_DIR" -type f -executable \
  \( -name '*_tests' -o -name '*_test' \) \
  | grep -v '/_deps/' | sort -u)

if [[ ${#TEST_BINS[@]} -eq 0 ]]; then
  warn "未发现任何测试二进制 (*_tests/*_test), 请确认 BUILD_*_TESTS 已开启并已编译"
fi

for bin in "${TEST_BINS[@]}"; do
  name="$(basename "$bin")"
  printf "  -> %-22s ... " "$name"
  if timeout "$TEST_TIMEOUT" "$bin" > "/tmp/run_${name}.log" 2>&1; then
    echo -e "${C_GREEN}PASS${C_RESET}"
  else
    code=$?
    [[ $code -eq 124 ]] && echo -e "${C_YELLOW}超时(kill)${C_RESET}" || echo -e "${C_YELLOW}exit=$code (有失败/崩溃, 仍产出覆盖率)${C_RESET}"
  fi
done

# -----------------------------------------------------------------------------
step "5/9  抓运行时覆盖率"
lcov -c -d "$BUILD_DIR" -o "$COV_DIR/runtime.info" "${LCOV_RC[@]}" -q 2>/dev/null
ok "runtime 文件数: $(grep -c '^SF:' "$COV_DIR/runtime.info")"

# -----------------------------------------------------------------------------
step "6/9  合并 baseline + runtime"
lcov -a "$COV_DIR/baseline.info" -a "$COV_DIR/runtime.info" \
     -o "$COV_DIR/combined.info" "${LCOV_RC[@]}" -q 2>/dev/null

# -----------------------------------------------------------------------------
step "7/9  过滤: 只保留 src/, 剔除 生成文件/测试/第三方/依赖"
lcov -e "$COV_DIR/combined.info" '*/src/*' -o "$COV_DIR/src.info" "${LCOV_RC[@]}" -q 2>/dev/null
lcov -r "$COV_DIR/src.info" \
     "*/${BUILD_DIR}/*" \
     '*/autogen_*' '*/moc_*.cpp' '*_qml_files*' '*_autogen/*' \
     '*/tests/*' '*/test/*' '*/3rdparty/*' '*/_deps/*' \
     -o "$COV_DIR/final.info" "${LCOV_RC[@]}" -q 2>/dev/null
ok "final 文件数: $(grep -c '^SF:' "$COV_DIR/final.info")  (残留 build 生成文件: $(grep -c "${BUILD_DIR}" "$COV_DIR/final.info"))"

# -----------------------------------------------------------------------------
step "8/9  生成 HTML 报告"
rm -rf "$COV_DIR/html"
genhtml "$COV_DIR/final.info" -o "$COV_DIR/html" "${LCOV_RC[@]}" -q 2>/dev/null
HTML_PATH="$PROJECT_DIR/$COV_DIR/html/index.html"
ok "HTML 报告: file://${HTML_PATH}"

# -----------------------------------------------------------------------------
step "9/9  覆盖率总览 + 按模块拆分"
echo -e "\n--- 全量 (整个 src/) ---"
lcov --summary "$COV_DIR/final.info" "${LCOV_RC[@]}" 2>/dev/null

echo -e "\n--- 按模块拆分 ---"
printf "  %-22s %s\n" "模块" "行覆盖率"
for mod in basekit logging netutil slotipc uasio fmt sslconf; do
  tmp="/tmp/cov_${mod}_$$.info"
  lcov -e "$COV_DIR/final.info" "*/src/infrastructure/$mod/*" -o "$tmp" "${LCOV_RC[@]}" -q 2>/dev/null
  s="$(lcov --summary "$tmp" "${LCOV_RC[@]}" 2>/dev/null | grep 'lines')"
  [[ -n "$s" ]] && printf "  infra/%-16s %s\n" "$mod" "$s" || printf "  infra/%-16s (未插桩/无数据)\n" "$mod"
  rm -f "$tmp"
done
for mod in lib compat common base configs singleton apps; do
  tmp="/tmp/cov_${mod}_$$.info"
  lcov -e "$COV_DIR/final.info" "*/src/$mod/*" -o "$tmp" "${LCOV_RC[@]}" -q 2>/dev/null
  s="$(lcov --summary "$tmp" "${LCOV_RC[@]}" 2>/dev/null | grep 'lines')"
  [[ -n "$s" ]] && printf "  %-22s %s\n" "$mod" "$s"
  rm -f "$tmp"
done

echo -e "\n${C_GREEN}完成。${C_RESET}打开报告查看: file://${HTML_PATH}"
