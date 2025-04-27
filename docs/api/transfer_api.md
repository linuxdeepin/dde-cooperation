+++
id = "transfer-api-20250427"
title = "TransferHelper API Documentation"
status = "draft"
doc_version = "1.0"
content_version = 1.0
audience = ["developers"]
last_reviewed = "2025-04-27"
template_schema_doc = ".ruru/templates/toml-md/09_documentation.README.md"
tags = ["api", "transfer", "bidirectional"]
related_tasks = ["TASK-BACKEND-20250427-100015"]
+++

# TransferHelper API Documentation

**Version:** 1.0 | **Content Revision:** 1.0 | **Last Reviewed:** 2025-04-27

## Introduction / Overview 🎯

This document describes the API for bidirectional file transfer functionality in the TransferHelper class.

## TransferHelper Class Methods 📝

### `sendFiles(const QString &ip, const QString &devName, const QStringList &fileList)`

Initiates file transfer to the specified device.

Parameters:
- `ip`: Target device IP address
- `devName`: Target device name
- `fileList`: List of file paths to transfer

### `transferStatus()`

Returns current transfer status as `TransferStatus` enum.

### `waitForConfirm()`

Shows confirmation dialog and waits for user response.

### `accepted()`

Called when transfer is accepted by the other party.

### `rejected()`

Called when transfer is rejected by the other party.

## TransferStatus Enum 🔢

```cpp
enum TransferStatus {
    Idle,        // No active transfer
    Connecting,  // Establishing connection
    Confirming,  // Waiting for confirmation
    Transfering, // Transfer in progress
    Failed       // Transfer failed
};
```

## Callback Interfaces 🔄

### `ButtonStateCallback`

```cpp
using ButtonStateCallback = std::function<bool(const QString &, const DeviceInfoPointer)>;
```

Determines if a button should be visible/enabled.

### `ClickedCallback`

```cpp
using ClickedCallback = std::function<void(const QString &, const DeviceInfoPointer)>;
```

Handles button click events.

## Example Usage 💻

```cpp
// Send files to device
TransferHelper::instance()->sendFiles("192.168.1.100", "Android-Device", {"/path/to/file1", "/path/to/file2"});

// Check transfer status
auto status = TransferHelper::instance()->transferStatus();
if (status == TransferHelper::Transfering) {
    // Handle transfer in progress
}
```

## Related Links 🔗

- [Source Code: transferhelper.cpp](src/lib/cooperation/core/net/helper/transferhelper.cpp)