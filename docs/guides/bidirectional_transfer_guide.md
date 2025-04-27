+++
id = "bidirectional-transfer-guide-20250427"
title = "Bidirectional File Transfer Guide"
status = "draft"
difficulty = "intermediate"
estimated_time = "~15 minutes"
target_audience = ["users", "testers"]
prerequisites = ["Basic knowledge of file operations"]
learning_objectives = [
    "Understand how to use bidirectional transfer",
    "Handle common error scenarios",
    "Monitor transfer progress"
]
last_tested = "2025-04-27"
tags = ["guide", "transfer", "bidirectional"]
related_tasks = ["TASK-BACKEND-20250427-100015"]
+++

# Bidirectional File Transfer Guide

**Difficulty:** Intermediate | **Est. Time:** ~15 minutes | **Last Tested:** 2025-04-27

## Introduction / Goal 🎯

This guide explains how to use the bidirectional file transfer feature to:
- Send files to another device
- Receive files from another device
- Monitor transfer progress
- Handle common error scenarios

## Prerequisites Checklist ✅

*   - [ ] Both devices are connected to the same network
*   - [ ] DDE Cooperation is installed on both devices
*   - [ ] You have file access permissions on both devices

## Step 1: Initiate File Transfer 📝

1. Open DDE Cooperation application
2. Select the target device from the list
3. Click "Send Files" button
4. Select files to transfer in the file dialog
5. Click "Open" to start the transfer process

```bash
# Example command line equivalent (for developers)
dde-cooperation-cli send --ip 192.168.1.100 --files /path/to/file1 /path/to/file2
```

## Step 2: Accept/Reject Incoming Transfer ➡️

When receiving files:
1. A notification will appear on the receiving device
2. Click the notification to open the transfer dialog
3. Choose "Accept" to begin receiving files or "Reject" to cancel
4. If accepted, select destination folder for the incoming files

## Step 3: Monitor Transfer Progress 📊

During transfer:
1. Progress bar shows overall completion percentage
2. Current file being transferred is highlighted
3. Transfer speed is displayed in real-time
4. Estimated time remaining is calculated and shown

## Verification / Check Your Work ✅

After transfer completes:
1. Verify all files were transferred successfully
2. Check file sizes match the originals
3. Confirm no errors were reported
4. Files should be in the specified destination folder

## Troubleshooting / Common Issues ❓

*   **Issue:** Transfer fails with "Connection lost" error
    *   **Solution:** Check network connectivity and try again

*   **Issue:** Files appear corrupted after transfer
    *   **Solution:** Verify network stability and retry the transfer

*   **Issue:** Transfer speed is very slow
    *   **Solution:** Check for network congestion or switch to a faster connection

## Summary / Next Steps 💡

You've learned how to:
- Initiate file transfers between devices
- Accept incoming transfers
- Monitor transfer progress
- Troubleshoot common issues

Next steps:
- Try transferring different file types
- Test with larger files to evaluate performance
- Review API documentation for advanced features

## Related Links 🔗

- [TransferHelper API Documentation](api/transfer_api.md)
- [Source Code: transferhelper.cpp](src/lib/cooperation/core/net/helper/transferhelper.cpp)