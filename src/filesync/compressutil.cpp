// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "compressutil.h"

#include <zstd.h>
#include <filesystem>

using namespace std;

namespace fs = std::filesystem;

void *CompressUtil::malloc_buffer(size_t size)
{
    void *const buff = malloc(size);
    if (buff) return buff;
    /* error */
    perror("malloc:");
    return nullptr;
}

void CompressUtil::compressFolder(const std::string &folderPath, std::ostream &outputStream, int cLevel)
{
    size_t const buffInSize = ZSTD_CStreamInSize();    /* can always read one full block */
    void  *const buffIn  = malloc_buffer(buffInSize);
    size_t const buffOutSize = ZSTD_CStreamOutSize();  /* can always flush a full block */
    void  *const buffOut = malloc_buffer(buffOutSize);
    ZSTD_CStream *const cstream = ZSTD_createCStream();
    if (cstream == NULL) { fprintf(stderr, "ZSTD_createCStream() error \n"); exit(10); }
    size_t const initResult = ZSTD_initCStream(cstream, cLevel);
    if (ZSTD_isError(initResult)) { fprintf(stderr, "ZSTD_initCStream() error : %s \n", ZSTD_getErrorName(initResult)); exit(11); }
    size_t read, toRead = buffInSize;

    for (const auto &entry : fs::recursive_directory_iterator(folderPath)) {
        // fs::path relativePath = fs::relative(entry.path(), folderPath);

        // // 将文件名和相对路径写入压缩流
        // std::string fileInfo = relativePath.string() + "\n";  // 使用换行符分隔文件信息，便于后续解析
        // outputStream.write(fileInfo.c_str(), fileInfo.size());

        if (!entry.is_regular_file()) {
            continue;
        }

        std::ifstream file(entry.path(), std::ios::binary);
        if (file) {

            size_t read, toRead = buffInSize;
            while ((read = file.readsome((char *)buffIn, toRead)) > 0) {
                ZSTD_inBuffer input = { buffIn, read, 0 };
                while (input.pos < input.size) {
                    ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
                    toRead = ZSTD_compressStream(cstream, &output, &input);    /* toRead is guaranteed to be <= ZSTD_CStreamInSize() */
                    if (ZSTD_isError(toRead)) { fprintf(stderr, "ZSTD_compressStream() error : %s \n", ZSTD_getErrorName(toRead)); exit(12); }
                    if (toRead > buffInSize) toRead = buffInSize;   /* Safely handle case when `buffInSize` is manually changed to a value < ZSTD_CStreamInSize()*/
                    outputStream.write((char *)buffOut, output.pos);
                }
            }
            file.close();
        }
    }

    ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
    size_t const remainingToFlush = ZSTD_endStream(cstream, &output);   /* close frame */
    if (remainingToFlush) { fprintf(stderr, "not fully flushed"); exit(13); }
    outputStream.write((char *)buffOut, output.pos);

    ZSTD_freeCStream(cstream);
    free(buffIn);
    free(buffOut);
}

void CompressUtil::decompressFolder(std::ifstream &compressedStream, const std::string &outputFolder)
{
    size_t const buffInSize = ZSTD_DStreamInSize();
    void  *const buffIn  = malloc_buffer(buffInSize);
    size_t const buffOutSize = ZSTD_DStreamOutSize();  /* Guarantee to successfully flush at least one complete compressed block in all circumstances. */
    void  *const buffOut = malloc_buffer(buffOutSize);

    ZSTD_DStream *const dstream = ZSTD_createDStream();
    if (dstream == NULL) { fprintf(stderr, "ZSTD_createDStream() error \n"); exit(10); }

    /* In more complex scenarios, a file may consist of multiple appended frames (ex : pzstd).
    *  The following example decompresses only the first frame.
    *  It is compatible with other provided streaming examples */
    size_t const initResult = ZSTD_initDStream(dstream);
    if (ZSTD_isError(initResult)) { fprintf(stderr, "ZSTD_initDStream() error : %s \n", ZSTD_getErrorName(initResult)); exit(11); }

    // 从压缩流中解析出文件名和相对路径
    // std::string fileInfo;
    // std::getline(inFile, fileInfo);
    // fileInfo 包含了文件的相对路径，可以根据需要解析成文件名和文件夹名等元数据

    // 解压的数据写入到输出文件夹中
    std::ofstream outputFile(outputFolder + "/1.mp4", std::ios::binary); // | std::ios::app)

    size_t read, toRead = initResult;
    while ((read = compressedStream.readsome((char *)buffIn, toRead))) {
        ZSTD_inBuffer input = { buffIn, read, 0 };
        while (input.pos < input.size) {
            ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
            toRead = ZSTD_decompressStream(dstream, &output, &input);   /* toRead : size of next compressed block */
            if (ZSTD_isError(toRead)) { fprintf(stderr, "ZSTD_decompressStream() error : %s \n", ZSTD_getErrorName(toRead)); exit(12); }
            // fwrite_orDie(buffOut, output.pos, fout);

            // 解压的数据写入到输出文件夹中
            // std::ofstream outputFile(outputFolder + "/outputFile.txt", std::ios::binary | std::ios::app);
            outputFile.write((char *)buffOut, output.pos);
        }
    }
    outputFile.close();

    ZSTD_freeDStream(dstream);

    free(buffIn);
    free(buffOut);
}

//int main(int argc, const char** argv)
//{
//    const char* const exeName = argv[0];
//    if (argc!=2) {
//        printf("wrong arguments\n");
//        printf("usage:\n");
//        printf("%s FILE\n", exeName);
//        return 1;
//    }
//    const char* const inFilename = argv[1];
//    // const char* const outFilename = createOutFilename_orDie(inFilename);
//    // compressFile_orDie(inFilename, outFilename, 1);

//#if 1
//    std::ofstream compressedStream;
//    compressedStream.open("compressed.zst", std::ios::binary);

//    compressFolder(inFilename, compressedStream, 1);

//    compressedStream.close();
//#endif

//    // 创建输出文件夹
//    fs::create_directories("output_folder");

//    // 打开压缩文件
//    std::ifstream compressedStream1;
//    compressedStream1.open("compressed.zst", std::ios::binary);
//    if (!compressedStream1) {
//        std::cerr << "Failed to open the compressed file." << std::endl;
//        return 1;
//    }

//    decompressFolder(compressedStream1, "output_folder");
//    compressedStream1.close();

//    return 0;
//}
