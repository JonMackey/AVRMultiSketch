/*******************************************************************************
	License
	****************************************************************************
	This program is free software; you can redistribute it
	and/or modify it under the terms of the GNU General
	Public License as published by the Free Software
	Foundation; either version 3 of the License, or
	(at your option) any later version.
 
	This program is distributed in the hope that it will
	be useful, but WITHOUT ANY WARRANTY; without even the
	implied warranty of MERCHANTABILITY or FITNESS FOR A
	PARTICULAR PURPOSE. See the GNU General Public
	License for more details.
 
	Licence can be viewed at
	http://www.gnu.org/licenses/gpl-3.0.txt

	Please maintain this license information along with authorship
	and copyright notices in any redistribution of this code
*******************************************************************************/
//
//  MainWindowController.h
//  AVRMultiSketch
//
//  Created by Jon on 11/2/18.
//  Copyright © 2018 Jon Mackey. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <ORSSerial/ORSSerial.h>
#import "AVRMultiSketchLogViewController.h"
#import "AVRMultiSketchTableViewController.h"

NS_ASSUME_NONNULL_BEGIN

@interface MainWindowController : NSWindowController <NSUserNotificationCenterDelegate, NSPathControlDelegate>
{
	IBOutlet NSView *sketchesView;
	IBOutlet NSView *serialView;
	IBOutlet NSTextField *summaryTextField;
	IBOutlet NSPathControl *arduinoPathControl;
	IBOutlet NSPathControl *tempFolderPathControl;
	IBOutlet NSPathControl *packagesFolderPathControl;
}
- (IBAction)open:(id)sender;
- (IBAction)add:(id)sender;
- (IBAction)save:(id)sender;
- (IBAction)saveas:(id)sender;
- (IBAction)verify:(id)sender;
- (IBAction)uploadHex:(id)sender;
- (IBAction)exportHex:(id)sender;

@property (nonatomic, strong) NSURL *appTempFolderURL;
@property (nonatomic, strong) NSURL *arduinoURL;
@property (nonatomic, strong) NSURL *tempFolderURL;
@property (nonatomic, strong) NSURL *packagesFolderURL;
@property (nonatomic, strong) AVRMultiSketchLogViewController *multiAppLogViewController;
@property (nonatomic, strong) AVRMultiSketchTableViewController *multiAppTableViewController;
@property (nonatomic, strong) ORSSerialPortManager *serialPortManager;
@property (nonatomic, strong) ORSSerialPort *_Nullable serialPort;

- (void)doOpen:(NSURL*)inDocURL;

@end

NS_ASSUME_NONNULL_END
