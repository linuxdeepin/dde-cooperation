+++
id = "transfer-feature-20250427"
title = "File Transfer Feature Documentation"
status = "draft"
doc_version = "1.0"
audience = ["developers", "users"]
last_updated = "2025-04-27"
tags = ["feature", "transfer", "bidirectional"]
related_docs = [
    "docs/api/transfer_api.md",
    "docs/guides/bidirectional_transfer_guide.md"
]
+++

# File Transfer Feature Documentation

## Overview

The bidirectional file transfer feature allows:
- Sending files between devices
- Receiving files from other devices
- Monitoring transfer progress
- Handling transfer errors

## Architecture

![Transfer Architecture Diagram]()

### Components:
1. **TransferHelper**: Core transfer logic
2. **Network Layer**: Handles device communication
3. **UI Components**: Progress display and user interaction

## API Reference

### TransferHelper Class

#### Methods:
- `sendFiles(ip, devName, fileList)`: Initiate transfer
- `transferStatus()`: Get current transfer state
- `waitForConfirm()`: Show confirmation dialog
- `accepted()`: Handle transfer acceptance
- `rejected()`: Handle transfer rejection

#### TransferStatus Enum:
```cpp
enum TransferStatus {
    Idle,        // No active transfer
    Connecting,  // Establishing connection
    Confirming,  // Waiting for confirmation
    Transfering, // Transfer in progress
    Failed       // Transfer failed
};
```

## User Guide

### Prerequisites
- [ ] Both devices on same network
- [ ] DDE Cooperation installed
- [ ] File access permissions

### Sending Files
1. Open DDE Cooperation
2. Select target device
3. Click "Send Files"
4. Select files
5. Click "Open" to start

### Receiving Files
1. Accept incoming transfer notification
2. Choose destination folder
3. Monitor progress

### Command Line Usage
```bash
dde-cooperation-cli send --ip 192.168.1.100 --files /path/to/file1 /path/to/file2
```

## Advanced Features

### Resume Capability
- Automatically resumes interrupted transfers
- Verifies file integrity
- Continues from last successful chunk

### Progress Tracking
- Real-time speed calculation
- Estimated time remaining
- Per-file progress indicators

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Connection lost | Check network connectivity |
| Corrupted files | Verify network stability and retry |
| Slow transfer | Check for network congestion |

## Related Resources
- [API Documentation](docs/api/transfer_api.md)
- [User Guide](docs/guides/bidirectional_transfer_guide.md)
- [Source Code](src/lib/cooperation/core/net/helper/transferhelper.cpp)