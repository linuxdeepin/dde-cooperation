# UOS遷移工具|deepin-data-transfer|

## 概述

UOS遷移工具是一款資料遷移工具，可以一鍵將您的個人資料和應用資料從Windows端傳輸到UOS端，幫助您實現無縫更換系統。

## 使用入門

您可以透過以下方式啟動、退出UOS遷移工具，或建立捷徑。

### 啟動UOS遷移工具

1. 單擊工具列上的啟動器圖示![deepin_launcher](../common/deepin_launcher.svg) ，進入啟動器介面。
2. 上下滾動滑鼠滾輪瀏覽或透過搜尋功能，找到UOS遷移工具圖示![deepin_data_transfer](../common/deepin-data-transfer.svg)，單擊啟動。
3. 右鍵單擊圖示 ![deepin_data_transfer](../common/deepin-data-transfer.svg)，您可以執行以下操作：
   - 單擊 **發送到桌面**，在桌面建立捷徑。
   - 單擊 **發送到工具列**，將應用程式固定到工具列。
   - 單擊 **開機自動啟動**，將應用程式添加到開機啟動項，在電腦開機時自動啟動該應用程式。

![index](fig/index.png)

## 建立連接

### 連接準備

在開始遷移前，請確保：

1. 發送端(Windows)和接收端(UOS)同時執行UOS遷移工具；
2. 兩台裝置處於同一區域網路內；
3. 網路連接穩定。

### 連接步驟

1. 在UOS端啟動遷移工具後，系統將自動顯示本機IP位址和連接密碼；
2. 在Windows端輸入UOS端的IP位址和連接密碼；
3. 單擊 **連接** 按鈕發送連接請求。

> ![attention](../common/attention.svg) 注意：每次連接僅支援一對裝置間的資料傳輸。

**UOS端：**

![connect_uos](fig/connect_uos.png)

**Windows端：**

![connect_windows](fig/connect_windows.png)

## 選擇遷移資料

成功建立連接後：

1. UOS端將顯示等待傳輸狀態；
2. Windows端將顯示可選擇遷移的資料列表；
3. 在Windows端勾選需要遷移的資料類型，統將自動計算並顯示所選資料的資料量及資料大小。

**UOS端：**

![data_uos](fig/data_uos.png)

**Windows端：**

![data_windows](fig/data_windows.png)

## 執行資料傳輸

在Windows端確認遷移資料範圍後，單擊 **開始遷移** 進行資料傳輸，並展示傳輸進度。資料傳輸時間取決於頻寬及需要傳輸的檔案大小。

> ![tips](../common/tips.svg)若您在傳輸過程中出現網路問題導致傳輸中斷，待重新建立連線成功後，您可以單擊 **繼續傳輸** 繼續上次的傳輸任務。

## 資料遷移完成

資料傳輸完成後，您可以在UOS端查看資料遷移結果。傳輸完成的資料，將被存放在您的/home目錄下。

![result](fig/result.png)

## 主選單

在主選單中，您可以切換視窗主題、查看說明手冊、退出UOS遷移工具等。

### 主題

視窗主題包含淺色主題、深色主題和系統主題。

1. 在UOS遷移工具主介面，單擊 ![icon_menu](../common/icon_menu.svg)。
2. 選擇 **主題**，選擇一種主題顏色。

### 幫助

查看說明手冊，進一步了解和使用UOS遷移工具。

1. 在UOS遷移工具主介面，單擊 ![icon_menu](../common/icon_menu.svg)。
2. 選擇 **幫助**。
3. 查看UOS遷移工具的詳細幫助手冊。

### 關於

1. 在UOS遷移工具主介面，單擊 ![icon_menu](../common/icon_menu.svg)。
2. 選擇 **關於**。
3. 查看UOS遷移工具的版本資訊和功能介紹。

### 退出

1. 在UOS 遷移工具主介面，單擊 ![icon_menu](../common/icon_menu.svg)。
2. 選擇 **退出**。
