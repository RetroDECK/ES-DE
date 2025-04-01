//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  BluetoothStatusApple.m
//
//  Gets the Bluetooth adapter status on macOS.
//

#import "BluetoothStatusApple.h"

#import <IOBluetooth/IOBluetooth.h>

int getBluetoothStatus()
{
    IOBluetoothHostController* hciController = [IOBluetoothHostController defaultController];

    if (hciController != NULL && hciController.powerState)
        return 1;
    else
        return 0;
}
