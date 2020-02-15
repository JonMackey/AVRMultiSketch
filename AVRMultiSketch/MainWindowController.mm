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
//  MainWindowController.m
//  AVRMultiSketch
//
//  Created by Jon on 11/2/18.
//  Copyright © 2018 Jon Mackey. All rights reserved.
//

#import "MainWindowController.h"
#import "ArduinoAppOpenDelegate.h"
#include "AVRElfFile.h"
#include "ConfigurationFile.h"
#include "FileInputBuffer.h"
#include "JSONElement.h"

// Defining AVR_OBJ_DUMP will run avr-objdump for all elf files.
// Saved as xxxM.ino.elf.txt, where xxx is the sketch name.
#define AVR_OBJ_DUMP	1

@interface MainWindowController ()

@end

@implementation MainWindowController

NSString *const kNSUserNotificationCenter = @"NSUserNotificationCenter";
#if SANDBOX_ENABLED
NSString *const kTempFolderURLBMKey = @"tempFolderURLBM";
NSString *const kPackagesFolderURLBMKey = @"packagesFolderURLBM";
NSString *const kArduinoAppURLBMKey = @"arduinoAppURLBM";
NSString *const kSavedSketchesURLBMKey = @"savedSketchesURLBM";
#else
NSString *const kArduinoAppPathKey = @"arduinoAppPath";
NSString *const kSavedSketchesPathKey = @"savedSketchesPath";
#endif
NSString *const kArduinoBundleIdentifier = @"cc.arduino.Arduino";
NSString *const kPortNameKey = @"portName";
NSString *const kSelectPrompt = @"Select";
extern NSString *const kNameKey;
extern NSString *const kLengthKey;
extern NSString *const kStartKey;
extern NSString *const kDeviceNameKey;
//NSString *const kSourceBMKey = @"sourceBM";
NSString *const kTempURLKey = @"tempURL";
NSString *const kTempCopyURLKey = @"tempCopyURL";
NSString *const kFQBNKey = @"FQBN";
struct SMenuItemDesc
{
	NSInteger	mainMenuTag;
	NSInteger	subMenuTag;
    SEL action;
};

SMenuItemDesc	menuItems[] = {
	{1,10, @selector(open:)},
	{1,11, @selector(add:)},
	{1,13, @selector(save:)},
	{1,14, @selector(saveas:)},
	{3,30, @selector(verify:)},
	{3,31, @selector(uploadHex:)},
	{3,32, @selector(exportHex:)}
};

- (void)windowDidLoad
{
    [super windowDidLoad];
	{
		const SMenuItemDesc*	miDesc = menuItems;
		const SMenuItemDesc*	miDescEnd = &menuItems[sizeof(menuItems)/sizeof(SMenuItemDesc)];
		for (; miDesc < miDescEnd; miDesc++)
		{
			NSMenuItem *menuItem = [[[NSApplication sharedApplication].mainMenu itemWithTag:miDesc->mainMenuTag].submenu itemWithTag:miDesc->subMenuTag];
			if (menuItem)
			{
				// Assign this object as the target.
				menuItem.target = self;
				menuItem.action = miDesc->action;
			}
		}
	}

	self.serialPortManager = [ORSSerialPortManager sharedSerialPortManager];

	/*
	*	Setup notification of when ports are connected/disconnected
	*/
	{
		NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
		[nc addObserver:self selector:@selector(serialPortsWereConnected:) name:ORSSerialPortsWereConnectedNotification object:nil];
		[nc addObserver:self selector:@selector(serialPortsWereDisconnected:) name:ORSSerialPortsWereDisconnectedNotification object:nil];
		[[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:self];
	}
	/*
	*	Initialize the port popup.
	*/
	{
		NSString*	portName = [[NSUserDefaults standardUserDefaults] objectForKey:kPortNameKey];
		if (portName && portName.length)
		{
			NSUInteger index = [self.serialPortManager.availablePorts indexOfObjectPassingTest:
				^BOOL(ORSSerialPort* inSerialPort, NSUInteger inIndex, BOOL *outStop)
				{
					return ([inSerialPort.name isEqualToString:portName]);
				}];
			if (index != NSNotFound)
			{
				self.serialPort = self.serialPortManager.availablePorts[index];
			}
		}
	}

	if (self.multiAppTableViewController == nil)
	{
		_multiAppTableViewController = [[AVRMultiSketchTableViewController alloc] initWithNibName:@"AVRMultiSketchTableViewController" bundle:nil];
		// embed the current view to our host view
		[sketchesView addSubview:[self.multiAppTableViewController view]];
		
		// make sure we automatically resize the controller's view to the current window size
		[[self.multiAppTableViewController view] setFrame:[sketchesView bounds]];
	}

	if (self.multiAppLogViewController == nil)
	{
		_multiAppLogViewController = [[AVRMultiSketchLogViewController alloc] initWithNibName:@"AVRMultiSketchLogViewController" bundle:nil];
		// embed the current view to our host view
		[serialView addSubview:[self.multiAppLogViewController view]];

		// make sure we automatically resize the controller's view to the current window size
		[[self.multiAppLogViewController view] setFrame:[serialView bounds]];
	}
	[self verifyAppTempFolder];
	[self verifyTempFolder];
	[self verifyPackagesFolder];
	[self verifyArduinoApp];
}

/******************************** awakeFromNib ********************************/
- (void)awakeFromNib
{
	[super awakeFromNib];
}

#pragma mark - Path Popup support
/**************************** willDisplayOpenPanel ****************************/
- (void)pathControl:(NSPathControl *)pathControl willDisplayOpenPanel:(NSOpenPanel *)openPanel
{
	if (pathControl.tag == 1)
	{
		ArduinoAppOpenDelegate* arduinoAppOpenDelegate = [[ArduinoAppOpenDelegate alloc] init];
		NSArray* appFolderURLs =[[NSFileManager defaultManager] URLsForDirectory:NSApplicationDirectory inDomains:NSLocalDomainMask];
		[openPanel setCanChooseDirectories:NO];
		[openPanel setCanChooseFiles:YES];
		[openPanel setAllowsMultipleSelection:NO];
		openPanel.directoryURL = appFolderURLs[0];
		openPanel.delegate = arduinoAppOpenDelegate;
		openPanel.message = @"Locate the Arduino Application";
		openPanel.prompt = kSelectPrompt;
	}
}

/******************************* willPopUpMenu ********************************/
- (void)pathControl:(NSPathControl *)pathControl willPopUpMenu:(NSMenu *)menu
{
	if (pathControl.tag == 2 ||
		pathControl.tag == 3)
	{
		NSMenuItem *openTemporaryFolderMenuItem = [[NSMenuItem alloc]initWithTitle:@"Show in Finder" action:@selector(showInFinder:) keyEquivalent:[NSString string]];
		openTemporaryFolderMenuItem.target = self;
		openTemporaryFolderMenuItem.tag = pathControl.tag;
		[menu insertItem:[NSMenuItem separatorItem] atIndex:0];
		[menu insertItem:openTemporaryFolderMenuItem atIndex:0];
	}
}

/******************************* showInFinder *********************************/
- (IBAction)showInFinder:(id)sender
{
	NSURL*	folderURL = nil;
	switch (((NSMenuItem*)sender).tag)
	{
		case 2:
			folderURL = _tempFolderURL;
			break;
		case 3:
			folderURL = _packagesFolderURL;
			break;
	}
	if (folderURL)
	{
	#if SANDBOX_ENABLED
			if ([folderURL startAccessingSecurityScopedResource])
			{
				[[NSWorkspace sharedWorkspace] openURL:folderURL];
				[folderURL stopAccessingSecurityScopedResource];
			}
		}
	#else
		[[NSWorkspace sharedWorkspace] openURL:folderURL];
	#endif
	}
}

#pragma mark - Notifications

- (void)serialPortsWereConnected:(NSNotification *)notification
{
	NSArray *connectedPorts = [notification userInfo][ORSConnectedSerialPortsKey];
	//NSLog(@"Ports were connected: %@", connectedPorts);
	[_multiAppLogViewController postInfoString: [NSString stringWithFormat:@"Ports were connected: %@", connectedPorts]];
	[self postUserNotificationForConnectedPorts:connectedPorts];
}

- (void)serialPortsWereDisconnected:(NSNotification *)notification
{
	NSArray *disconnectedPorts = [notification userInfo][ORSDisconnectedSerialPortsKey];
	//NSLog(@"Ports were disconnected: %@", disconnectedPorts);
	[_multiAppLogViewController postInfoString: [NSString stringWithFormat:@"Ports were disconnected: %@", disconnectedPorts]];
	[self postUserNotificationForDisconnectedPorts:disconnectedPorts];
	
}

- (void)postUserNotificationForConnectedPorts:(NSArray *)connectedPorts
{
	if (!NSClassFromString(kNSUserNotificationCenter)) return;
	
	NSUserNotificationCenter *unc = [NSUserNotificationCenter defaultUserNotificationCenter];
	for (ORSSerialPort *port in connectedPorts)
	{
		NSUserNotification *userNote = [[NSUserNotification alloc] init];
		userNote.title = NSLocalizedString(@"Serial Port Connected", @"Serial Port Connected");
		NSString *informativeTextFormat = NSLocalizedString(@"Serial Port %@ was connected to your Mac.", @"Serial port connected user notification informative text");
		userNote.informativeText = [NSString stringWithFormat:informativeTextFormat, port.name];
		userNote.soundName = nil;
		[unc deliverNotification:userNote];
	}
}

- (void)postUserNotificationForDisconnectedPorts:(NSArray *)disconnectedPorts
{
	if (!NSClassFromString(kNSUserNotificationCenter)) return;
	
	NSUserNotificationCenter *unc = [NSUserNotificationCenter defaultUserNotificationCenter];
	for (ORSSerialPort *port in disconnectedPorts)
	{
		NSUserNotification *userNote = [[NSUserNotification alloc] init];
		userNote.title = NSLocalizedString(@"Serial Port Disconnected", @"Serial Port Disconnected");
		NSString *informativeTextFormat = NSLocalizedString(@"Serial Port %@ was disconnected from your Mac.", @"Serial port disconnected user notification informative text");
		userNote.informativeText = [NSString stringWithFormat:informativeTextFormat, port.name];
		userNote.soundName = nil;
		[unc deliverNotification:userNote];
		self.serialPort = nil;
	}
}

/***************************** setSerialPort **********************************/
/*
*	Override of setter function for _serialPort
*/
- (void)setSerialPort:(ORSSerialPort *)port
{
	if (port != _serialPort)
	{
		_serialPort = port;
		if (port)
		{
			[[NSUserDefaults standardUserDefaults] setObject:_serialPort.name forKey:kPortNameKey];
		}
	}
}

/****************************** verifyTempFolder ******************************/
/*
*	This makes sure the previous temporary folder still exists.  If it no
*	longer exists the user will be asked to select it.
*/
- (void)verifyTempFolder
{
#if SANDBOX_ENABLED
	NSString*	tempFolderPath = [[[NSFileManager defaultManager] temporaryDirectory].path stringByDeletingLastPathComponent];
	NSURL*	tempFolderURL = NULL;
	NSData*	tempFolderURLBM = [[NSUserDefaults standardUserDefaults] objectForKey:kTempFolderURLBMKey];
	if (tempFolderURLBM)
	{
		tempFolderURL = [NSURL URLByResolvingBookmarkData: tempFolderURLBM
				options:NSURLBookmarkResolutionWithoutUI+NSURLBookmarkResolutionWithoutMounting+NSURLBookmarkResolutionWithSecurityScope
						relativeToURL:NULL bookmarkDataIsStale:NULL error:NULL];
	}
	// stringByStandardizingPath will remove /private from the path. NSFileManager
	// seems to be standardizing the URL it returns.
	BOOL	success = tempFolderURL && [[tempFolderURL.path stringByStandardizingPath] isEqualToString:tempFolderPath];
	if (!success)
	{
		NSOpenPanel*	openPanel = [NSOpenPanel openPanel];
		if (openPanel)
		{
			[openPanel setCanChooseDirectories:YES];
			[openPanel setCanChooseFiles:NO];
			[openPanel setAllowsMultipleSelection:NO];
			openPanel.showsHiddenFiles = YES;
			openPanel.directoryURL = [NSURL fileURLWithPath:tempFolderPath isDirectory:YES];
			openPanel.message = @"Because of sandbox issues, we need to get permission to access the temporary items folder. Please press Select.";
			openPanel.prompt = kSelectPrompt;
			//[openPanel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result)
			{
				if ([openPanel runModal] == NSModalResponseOK)
				{
					NSArray* urls = [openPanel URLs];
					if ([urls count] == 1)
					{
						NSError*	error;
						tempFolderURL = urls[0];
						if ([[tempFolderURL.path stringByStandardizingPath] isEqualToString:tempFolderPath])
						{
							tempFolderURLBM = [tempFolderURL bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
									includingResourceValuesForKeys:NULL relativeToURL:NULL error:&error];
							[[NSUserDefaults standardUserDefaults] setObject:tempFolderURLBM forKey:kTempFolderURLBMKey];
							success = !error;
							if (error)
							{
								NSLog(@"-verifyTempFolder- %@\n", error);
							}
						} else
						{
							[[NSUserDefaults standardUserDefaults] removeObjectForKey:kTempFolderURLBMKey];
							NSAlert *alert = [[NSAlert alloc] init];
							[alert setMessageText:@"Temporary Folder"];
							[alert setInformativeText:@"The temporary items folder was not selected. "
								"Restart the application then simply press Select."];
							[alert addButtonWithTitle:@"OK"];
							[alert setAlertStyle:NSAlertStyleWarning];
							[alert runModal];
						}
					}
				}
			}
		}
	}
#else
	BOOL	success = YES;
	NSURL* tempFolderURL = [[NSFileManager defaultManager] temporaryDirectory];
#endif
	if (success)
	{
		_tempFolderURL = tempFolderURL;
		tempFolderPathControl.URL = tempFolderURL;
	}
}

/****************************** verifyArduinoApp ******************************/
/*
*	This makes sure the previously selected Arduino app still exists.  If it no
*	longer exists the user will be asked to select one.
*/
- (void)verifyArduinoApp
{
#if SANDBOX_ENABLED
	NSURL*	arduinoAppURL = NULL;
	NSData*	arduinoAppURLBM = [[NSUserDefaults standardUserDefaults] objectForKey:kArduinoAppURLBMKey];
	if (arduinoAppURLBM)
	{
		arduinoAppURL = [NSURL URLByResolvingBookmarkData: arduinoAppURLBM
				options:NSURLBookmarkResolutionWithoutUI+NSURLBookmarkResolutionWithoutMounting+NSURLBookmarkResolutionWithSecurityScope
						relativeToURL:NULL bookmarkDataIsStale:NULL error:NULL];
	}
#else
	NSString*	arduinoAppPath = [[NSUserDefaults standardUserDefaults] objectForKey:kArduinoAppPathKey];
	NSURL*	arduinoAppURL = arduinoAppPath ? [NSURL fileURLWithPath:arduinoAppPath isDirectory:NO] : nil;
	if (arduinoAppURL)
	{
		arduinoPathControl.URL = arduinoAppURL;
	}
#endif
	if (!arduinoAppURL)
	{
		NSArray* appFolderURLs =[[NSFileManager defaultManager] URLsForDirectory:NSApplicationDirectory inDomains:NSLocalDomainMask];
		if ([appFolderURLs count] > 0)
		{
			ArduinoAppOpenDelegate* arduinoAppOpenDelegate = [[ArduinoAppOpenDelegate alloc] init];
			NSOpenPanel*	chooseAppPanel = [NSOpenPanel openPanel];
			if (chooseAppPanel)
			{
				[chooseAppPanel setCanChooseDirectories:NO];
				[chooseAppPanel setCanChooseFiles:YES];
				[chooseAppPanel setAllowsMultipleSelection:NO];
				chooseAppPanel.directoryURL = appFolderURLs[0];
				chooseAppPanel.delegate = arduinoAppOpenDelegate;
				chooseAppPanel.message = @"Locate the Arduino Application";
				chooseAppPanel.prompt = kSelectPrompt;
				if ([chooseAppPanel runModal] == NSModalResponseOK)
				{
#if SANDBOX_ENABLED
					arduinoAppURLBM = [[NSUserDefaults standardUserDefaults] objectForKey:kArduinoAppURLBMKey];
					arduinoAppURL = [NSURL URLByResolvingBookmarkData: arduinoAppURLBM
							options:NSURLBookmarkResolutionWithoutUI+NSURLBookmarkResolutionWithoutMounting+NSURLBookmarkResolutionWithSecurityScope
									relativeToURL:NULL bookmarkDataIsStale:NULL error:NULL];
#else
					NSString* arduinoAppPath = [[NSUserDefaults standardUserDefaults] objectForKey:kArduinoAppPathKey];
					arduinoAppURL = arduinoAppPath ? [NSURL fileURLWithPath:arduinoAppPath isDirectory:NO] : nil;
#endif
				}
			}
		}
	}
	_arduinoURL = arduinoAppURL;
	arduinoPathControl.URL = arduinoAppURL;
}

/**************************** verifyPackagesFolder ****************************/
/*
*	This makes sure the previous packages folder still exists.  If it no
*	longer exists the user will be asked to select it.
*/
- (void)verifyPackagesFolder
{
	BOOL	success = NO;
	NSURL*	packagesFolderURL = NULL;
	NSArray<NSURL*>* libraryFolderURLs =[[NSFileManager defaultManager] URLsForDirectory:NSLibraryDirectory inDomains:NSUserDomainMask];
	if (libraryFolderURLs.count > 0)
	{
#if SANDBOX_ENABLED
		// Kind of convoluted, but when you ask for the library folder in a sandboxed app you get the sandboxed folder
		// within the app's container.  To get the actual library folder you need to strip off 4 path components.
		// I know it's a kludge, but I don't know of an honest way of doing this.
		NSString*	packagesFolderPath = [[[[[[libraryFolderURLs objectAtIndex:0].path stringByDeletingLastPathComponent]
											stringByDeletingLastPathComponent] stringByDeletingLastPathComponent]
											stringByDeletingLastPathComponent] stringByAppendingPathComponent:@"Arduino15"];
		NSData*	packagesFolderURLBM = [[NSUserDefaults standardUserDefaults] objectForKey:kPackagesFolderURLBMKey];
		if (packagesFolderURLBM)
		{
			packagesFolderURL = [NSURL URLByResolvingBookmarkData: packagesFolderURLBM
					options:NSURLBookmarkResolutionWithoutUI+NSURLBookmarkResolutionWithoutMounting+NSURLBookmarkResolutionWithSecurityScope
							relativeToURL:NULL bookmarkDataIsStale:NULL error:NULL];
		}
		// stringByStandardizingPath will remove /private from the path. NSFileManager
		// seems to be standardizing the URL it returns.
		success = packagesFolderURL && [[packagesFolderURL.path stringByStandardizingPath] isEqualToString:packagesFolderPath];
		if (!success)
		{
			NSOpenPanel*	openPanel = [NSOpenPanel openPanel];
			if (openPanel)
			{
				[openPanel setCanChooseDirectories:YES];
				[openPanel setCanChooseFiles:NO];
				[openPanel setAllowsMultipleSelection:NO];
				openPanel.directoryURL = [NSURL fileURLWithPath:packagesFolderPath isDirectory:YES];
				openPanel.message = @"Because of sandbox issues, we need to get permission to access the Arduino15 folder. "
									"Please navigate to and select ~/Library/Arduino15.";
				openPanel.prompt = kSelectPrompt;
				//[openPanel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result)
				{
					if ([openPanel runModal] == NSModalResponseOK)
					{
						NSArray* urls = [openPanel URLs];
						if ([urls count] == 1)
						{
							NSError*	error;
							packagesFolderURL = urls[0];
							if ([[packagesFolderURL.path stringByStandardizingPath] isEqualToString:packagesFolderPath])
							{
								packagesFolderURLBM = [packagesFolderURL bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
										includingResourceValuesForKeys:NULL relativeToURL:NULL error:&error];
								[[NSUserDefaults standardUserDefaults] setObject:packagesFolderURLBM forKey:kPackagesFolderURLBMKey];
								success = !error;
								if (error)
								{
									NSLog(@"-verifyPackagesFolder- %@\n", error);
								}
							} else
							{
								[[NSUserDefaults standardUserDefaults] removeObjectForKey:kPackagesFolderURLBMKey];
								NSAlert *alert = [[NSAlert alloc] init];
								[alert setMessageText:@"Arduino Packages Folder"];
								[alert setInformativeText:@"The Arduino15 packages folder was not selected. "
									"Restart the application then simply press Select."];
								[alert addButtonWithTitle:@"OK"];
								[alert setAlertStyle:NSAlertStyleWarning];
								[alert runModal];
							}
						}
					}
				}
			}
		}
#else
		NSString*	packagesFolderPath = [[libraryFolderURLs objectAtIndex:0].path stringByAppendingPathComponent:@"Arduino15"];
		packagesFolderURL = [NSURL fileURLWithPath:packagesFolderPath isDirectory:YES];
		success = YES;
#endif
	}
	if (success)
	{
		_packagesFolderURL = packagesFolderURL;
		packagesFolderPathControl.URL = packagesFolderURL;
	}
}

/*********************************** add **************************************/
- (IBAction)add:(id)sender
{
}

/********************************** open **************************************/
- (IBAction)open:(id)sender
{
	NSURL*	baseURL = NULL;
#if SANDBOX_ENABLED
	NSData* savedSketchesBM = [[NSUserDefaults standardUserDefaults] objectForKey:kSavedSketchesURLBMKey];
	if (savedSketchesBM)
	{
		baseURL = [NSURL URLByResolvingBookmarkData:
	 					savedSketchesBM
	 						options:NSURLBookmarkResolutionWithoutUI+NSURLBookmarkResolutionWithoutMounting+NSURLBookmarkResolutionWithSecurityScope
	 							relativeToURL:NULL bookmarkDataIsStale:NULL error:NULL];
		baseURL = [NSURL fileURLWithPath:[[baseURL path] stringByDeletingLastPathComponent] isDirectory:YES];
	}
#else
	NSString* savedSketchesPath = [[NSUserDefaults standardUserDefaults] objectForKey:kSavedSketchesPathKey];
	baseURL = savedSketchesPath ? [NSURL fileURLWithPath:[savedSketchesPath stringByDeletingLastPathComponent] isDirectory:YES] : nil;
#endif
	NSOpenPanel*	openPanel = [NSOpenPanel openPanel];
	if (openPanel)
	{
		[openPanel setCanChooseDirectories:NO];
		[openPanel setCanChooseFiles:YES];
		[openPanel setAllowsMultipleSelection:NO];
		openPanel.directoryURL = baseURL;
		openPanel.allowedFileTypes = @[@"sfl2"];
		[openPanel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result)
		{
			if (result == NSModalResponseOK)
			{
				NSArray* urls = [openPanel URLs];
				if ([urls count] == 1)
				{
					[self doOpen:urls[0]];
				}
			}
		}];
	}
}

/********************************* doOpen *************************************/
- (void)doOpen:(NSURL*)inDocURL
{
	if ([_multiAppTableViewController setSketchesWithContentsOfURL:inDocURL])
	{
		[self newRecentSketchesDoc:inDocURL];
	}
}

/*********************************** save *************************************/
- (IBAction)save:(id)sender
{
#if SANDBOX_ENABLED
	NSData* savedSketchesBM = [[NSUserDefaults standardUserDefaults] objectForKey:kSavedSketchesURLBMKey];
	NSURL*	docURL = [NSURL URLByResolvingBookmarkData:
	 					savedSketchesBM
	 						options:NSURLBookmarkResolutionWithoutUI+NSURLBookmarkResolutionWithoutMounting+NSURLBookmarkResolutionWithSecurityScope
	 							relativeToURL:NULL bookmarkDataIsStale:NULL error:NULL];
#else
	NSString* savedSketchesPath = [[NSUserDefaults standardUserDefaults] objectForKey:kSavedSketchesPathKey];
	NSURL*	docURL = savedSketchesPath ? [NSURL fileURLWithPath:savedSketchesPath isDirectory:NO] : nil;
#endif
	if (docURL)
	{
		[self doSave:docURL];
	} else
	{
		[self saveas:sender];
	}
}

/********************************* saveas *************************************/
- (IBAction)saveas:(id)sender
{
#if SANDBOX_ENABLED
	NSData* savedSketchesBM = [[NSUserDefaults standardUserDefaults] objectForKey:kSavedSketchesURLBMKey];
	NSURL*	docURL = [NSURL URLByResolvingBookmarkData:
	 					savedSketchesBM
	 						options:NSURLBookmarkResolutionWithoutUI+NSURLBookmarkResolutionWithoutMounting+NSURLBookmarkResolutionWithSecurityScope
	 							relativeToURL:NULL bookmarkDataIsStale:NULL error:NULL];
#else
	NSString* savedSketchesPath = [[NSUserDefaults standardUserDefaults] objectForKey:kSavedSketchesPathKey];
	NSURL*	docURL = savedSketchesPath ? [NSURL fileURLWithPath:savedSketchesPath isDirectory:NO] : nil;
#endif
	NSURL*	baseURL = docURL ? [NSURL fileURLWithPath:[docURL.path stringByDeletingLastPathComponent] isDirectory:YES] : nil;
	NSString*	initialName = docURL ? [[[docURL path] lastPathComponent] stringByDeletingPathExtension] : @"Untitled";
	NSSavePanel*	savePanel = [NSSavePanel savePanel];
	if (savePanel)
	{
		savePanel.directoryURL = baseURL;
		savePanel.allowedFileTypes = @[@"sfl2"];
		savePanel.nameFieldStringValue = initialName;
		[savePanel beginSheetModalForWindow:self.window completionHandler:^(NSInteger result)
		{
			if (result == NSModalResponseOK)
			{
				NSURL* docURL = savePanel.URL;
				if (docURL)
				{
					[self doSave:docURL];
				}
			}
		}];
	}
}

/********************************* doSave *************************************/
- (void)doSave:(NSURL*)inDocURL
{
	if ([_multiAppTableViewController writeSketchesToURL:inDocURL])
	{
		[self newRecentSketchesDoc:inDocURL];

	}
}

/*************************** newRecentSketchesDoc *****************************/
- (void)newRecentSketchesDoc:(NSURL*)inDocURL
 {
#if SANDBOX_ENABLED
	NSError*	error;
	NSData* savedSketchesBM = [inDocURL bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
			includingResourceValuesForKeys:NULL relativeToURL:NULL error:&error];
	[[NSUserDefaults standardUserDefaults] setObject:savedSketchesBM forKey:kSavedSketchesURLBMKey];
#else
	[[NSUserDefaults standardUserDefaults] setObject:inDocURL.path forKey:kSavedSketchesPathKey];
#endif
	[[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:inDocURL];
}

/*********************************** exportHex *************************************/
- (IBAction)exportHex:(id)sender
{
}

/********************************* uploadHex **********************************/
- (IBAction)uploadHex:(id)sender
{
	if ([self doUploadHex])
	{
		[self logSuccess];
	}
}

/*********************************** verify ***********************************/
- (IBAction)verify:(id)sender
{
	BoardsConfigFiles configFiles;
	if ([self doVerify:configFiles])
	{
		[self logSuccess];
	}
}

/********************************* logSuccess *********************************/
-(void)logSuccess
{
	[[_multiAppLogViewController appendColoredString:_multiAppLogViewController.greenColor string:@"Success!\n"] post];
}

/********************************** clear *************************************/
- (IBAction)clear:(id)sender
{
	[_multiAppLogViewController clear:sender];
}

/********************************* elfPathFor *********************************/
+(const char*)elfPathFor:(NSDictionary*)inSketchRec forKey:(NSString*)inKey
{
	return([((NSURL*)[inSketchRec objectForKey:inKey]) URLByAppendingPathComponent:[inSketchRec[kNameKey] stringByAppendingPathExtension:@"elf"]].path.UTF8String);
}

/********************************* doUploadHex ********************************/
- (BOOL)doUploadHex
{
	BOOL success = self.serialPort != NULL;
	if (success)
	{
		BoardsConfigFiles configFiles;
		success = [self doVerify:configFiles];
		BoardsConfigFile*	primaryConfigFile = configFiles.GetPrimaryConfig();
		if (success && primaryConfigFile)
		{// serial.port upload.verbose "-v"
			primaryConfigFile->InsertKeyValue("upload.verbose", "");
			primaryConfigFile->InsertKeyValue("serial.port", self.serialPort.path.UTF8String);
			primaryConfigFile->Promote("tools.avrdude.");
			// Change the build path and project name so that it point to
			// the combined hex file
			// {build.path}/{build.project_name}.hex
			primaryConfigFile->InsertKeyValue("build.path", _appTempFolderURL.path.UTF8String);
			primaryConfigFile->InsertKeyValue("build.project_name", "combined");
#if 1//SANDBOX_ENABLED
			std::string value;
			uint32_t keysNotFound = 0;
			primaryConfigFile->ValueForKey("upload.pattern", value, keysNotFound);
			[self->_multiAppLogViewController postInfoString: [NSString stringWithFormat:
				@"\nRun the following in Terminal or a BBEdit worksheet:\n\n%s\n\n", value.c_str()]];

#else
			success = [self runShellForRecipe:"upload.pattern" configFile:primaryConfigFile];
			if (success)
			{
				[self->_multiAppLogViewController postInfoString: @"Done uploading."];
			}
#endif
		}
	} else
	{
		[_multiAppLogViewController postErrorString: @"No serial port is selected."];
	}
	return(success);
}

typedef std::vector<uint16_t> Uint16Vec;
/********************************** doVerify **********************************/
- (BOOL)doVerify:(BoardsConfigFiles&) outConfigFiles
{
	[_multiAppLogViewController clear:self];
	__block BOOL	success = NO;
	__block Uint16Vec	subSketchOffsets;
	__block uint32_t	offset = 0;
	// Verify that the Arduino app is accessible and is running
	if (_arduinoURL)
	{
		success = YES;
		NSArray*	runningArduinoApplications = [NSRunningApplication runningApplicationsWithBundleIdentifier:kArduinoBundleIdentifier];
		//if (runningArduinoApplications.count > 0)
		{
	#if 1
			NSUInteger numRunninArduinoApps = runningArduinoApplications.count;
			NSUInteger index = 0;
			for (; index < numRunninArduinoApps; index++)
			{
				NSRunningApplication* runningApp = runningArduinoApplications[index];
				//NSLog(@"%@", runningApp.bundleURL.path);
				if ([runningApp.bundleURL.path isEqual:self.arduinoURL.path])
				{
					break;
				}
			}
			if (index == numRunninArduinoApps)
	#else
			NSUInteger index = [runningArduinoApplications indexOfObjectPassingTest:
				^BOOL(NSRunningApplication* inApplication, NSUInteger inIndex, BOOL *outStop)
				{
					*outStop = [[inApplication bundleURL] isEqual:self.arduinoURL];
					return (*outStop);
				}];
			if (index == NSNotFound)
	#endif
			{
				success = NO;
				[_multiAppLogViewController postErrorString:
					@"The selected Arduino application must be running."];
			}
		}
	} else
	{
		[_multiAppLogViewController postErrorString: @"The Arduino application has not been selected."];
		success = NO;
	}
	if (success)
	{
		success = [self verifyAppTempFolder];
	}
	
	if (success &&
		!_tempFolderURL)
	{
		[_multiAppLogViewController postErrorString: @"Permission to use the temporary folder has not been granted."
		" Please restart this application then press Select when asked to."];
		success = NO;
	}
	
	// Clear this application's temporary folder
	{
		NSArray<NSURL*>* contentsOfDirectory = [[NSFileManager defaultManager] contentsOfDirectoryAtURL:
			_appTempFolderURL includingPropertiesForKeys:nil
			options:NSDirectoryEnumerationSkipsHiddenFiles error:nil];
		for (NSUInteger fCount = contentsOfDirectory.count; fCount; )
		{
			fCount--;
			[[NSFileManager defaultManager] removeItemAtURL:[contentsOfDirectory objectAtIndex:fCount] error:nil];
		}
	}

	if (success)
	{
		__block NSMutableArray<NSMutableDictionary*>*	sketches = _multiAppTableViewController.sketches;
		__block uint32_t	forwarderDataSize = 0;
		if (sketches.count)
		{
			[sketches enumerateObjectsUsingBlock:
				^void(NSMutableDictionary* inSketchRec, NSUInteger inIndex, BOOL *outStop)
				{
					[inSketchRec removeObjectForKey:kTempURLKey];
				}];
		} else
		{
			[_multiAppLogViewController postErrorString: @"Sketch list is empty."];
			success = NO;
		}
		
		// Verify that all of the sketches have been compiled
		// This is done by locating each of the corresponding sketch elf files in the temporary folder.
		//NSLog(@"%@", sketches);
#if SANDBOX_ENABLED
		if (success &&
			[_tempFolderURL startAccessingSecurityScopedResource])
		{
			if ([_arduinoURL startAccessingSecurityScopedResource])
			{
				if ([_packagesFolderURL startAccessingSecurityScopedResource])
#else
		if (success)
#endif
				{
					// For each temporary arduino folder, attempt to match up a
					// corresponding file.  When found, save the URL to this folder.
					// in the sketches dictionary for this sketch.
					{
						NSDirectoryEnumerator* tempFolderEnum = [[NSFileManager defaultManager] enumeratorAtURL:_tempFolderURL
							includingPropertiesForKeys:NULL options:NSDirectoryEnumerationSkipsHiddenFiles errorHandler:nil];
						NSURL* folderURL;
						[tempFolderEnum skipDescendants];
						bool	cacheFolderCopied = false;
						while ((folderURL = [tempFolderEnum nextObject]))
						{
							if ([folderURL hasDirectoryPath])
							{
								if ([[folderURL lastPathComponent] hasPrefix: @"arduino_build_"])
								{
									//NSLog(@"\t%@", [folderURL.path lastPathComponent]);
									[sketches enumerateObjectsUsingBlock:
										^void(NSMutableDictionary* inSketchRec, NSUInteger inIndex, BOOL *outStop)
										{
											/*
											*	If this sketch's temporary folder hasn't been located THEN
											*	see if it's this one.
											*/
											if (![inSketchRec objectForKey:kTempURLKey])
											{
												NSString* elfName = [inSketchRec[kNameKey] stringByAppendingPathExtension:@"elf"];
												NSURL*	elfURL = [folderURL URLByAppendingPathComponent:elfName];
												if ([elfURL checkResourceIsReachableAndReturnError:NULL])
												{
													[inSketchRec setObject:folderURL forKey:kTempURLKey];
													[self->_multiAppLogViewController postInfoString: [NSString stringWithFormat:@"%@ found in %@", elfName, [folderURL.path lastPathComponent]]];
													*outStop = YES;
												}
											}
										}];
								/*
								*	Else if this is an arduino cache folder, copy it
								*/
								} else if ([[folderURL lastPathComponent] hasPrefix: @"arduino_cache_"])
								{
									if (!cacheFolderCopied)
									{
										cacheFolderCopied = true;
										NSURL*	tempCopyURL = [_appTempFolderURL URLByAppendingPathComponent:@"cache"];
										NSError* error = nil;
										[[NSFileManager defaultManager] copyItemAtURL:folderURL toURL:tempCopyURL error:&error];
										if (error)
										{
											success = NO;
											[_multiAppLogViewController postErrorString: [NSString stringWithFormat:@"Unable to copy the Arduino cache folder\n%@\n", error]];
											break;
										}
									} else
									{
										success = NO;
										[self->_multiAppLogViewController postErrorString:
											@"More than one arduino_cache_xxx folder exists in the temporary folder.  "
											"This can happen if the Arduino IDE crashed at some point.  "
											"Quit the Arduino IDE then trash all folders that begin with \"arduino_\"."];
										[[NSWorkspace sharedWorkspace] openURL:_tempFolderURL];
										break;
									}
								}
							}
						}
					}
					__block NSString*	sketchesNotLocated = NULL;
					[sketches enumerateObjectsUsingBlock:
						^void(NSMutableDictionary* inSketchRec, NSUInteger inIndex, BOOL *outStop)
						{
							if ([inSketchRec objectForKey:kTempURLKey] == nil)
							{
								if (sketchesNotLocated)
								{
									sketchesNotLocated = [NSString stringWithFormat:@"%@\n\t%@", sketchesNotLocated, inSketchRec[kNameKey]];
								} else
								{
									sketchesNotLocated = [NSString stringWithFormat:@"\n\t%@", inSketchRec[kNameKey]];
								}
							}
						}];
					if (sketchesNotLocated)
					{
						[_multiAppLogViewController postErrorString: [NSString stringWithFormat:
							@"The following sketches have not been compiled using "
							"the current instance of the Arduino IDE:%@"
							"\n\nVerify/Compile each sketch listed above using the Arduino IDE.  "
							"The Arduino IDE Verify action creates files in the temporary folder "
							"required by this application.", sketchesNotLocated]];
						success = NO;
					} else
					{

						/*
						*	Create the configuration file(s) based on the FQBN(s)
						*	of each sketch. (generally one for all sketches)
						*/
						{
							NSMutableDictionary* sketchRec;
							NSUInteger	sketchCount = sketches.count;
							for (NSUInteger sketchIndex = 0; success && sketchIndex < sketchCount; sketchIndex++)
							{
								sketchRec = [sketches objectAtIndex:sketchIndex];
								success = [self copyArduinoTempFolderFor:sketchRec];
								if (success)
								{
									BoardsConfigFile* configFile = [self initializeFQBNConfigFor:sketchRec configFile:outConfigFiles];
									if (configFile)
									{
										[sketchRec setObject:[NSString stringWithUTF8String:configFile->GetFQBN().c_str()] forKey:kFQBNKey];
										std::string deviceName;
										configFile->RawValueForKey("build.mcu", deviceName);
										[sketchRec setObject:[NSString stringWithUTF8String:deviceName.c_str()] forKey:kDeviceNameKey];
										if (sketchIndex == 1)
										{
											outConfigFiles.SetPrimaryFQBN(configFile->GetFQBN());
										}
										continue;
									}
									success = NO;
								}
							}
						}
						/*
						*	At this point all of the elf files have been located.  Now extract
						*	all of the vectors used by the sub sketches to determine if the
						*	forwarder sketch can handle them.
						*
						*	This could be done by checking the symbol table vector symbols, but
						*	an easier and more reliable method is to scan the vector table
						*	starting at offset 4.  Any vector that doesn't jump to
						*	__bad_interrupt is implemented.  __bad_interrupt is
						*	different for each sketch, so the symbol for __bad_interrupt is
						*	extracted then a jmp __bad_interrupt instruction is created for each
						*	sketch and used to get the implemented vector indexes.
						*
						*	To save on flash there's a limited number of placeholder
						*	vector entries in the original forwarder sketch.  An error
						*	will be generated if additional vector placeholder entries
						*	need to be added.
						*/
						if (success)
						{
							__block IndexVec	forwarderVectors;
							__block IndexVec	reqSketchVectors;
							[sketches enumerateObjectsUsingBlock:
								^void(NSMutableDictionary* inSketchRec, NSUInteger inIndex, BOOL *outStop)
								{
									AVRElfFile	elfFile;
									if (elfFile.ReadFile([MainWindowController elfPathFor:inSketchRec forKey:kTempURLKey]) &&
										elfFile.GetVectorIndexes(inIndex ? reqSketchVectors : forwarderVectors))
									{
										uint32_t	length = elfFile.GetFlashUsed();
										[self->_multiAppTableViewController setData:offset length:length forIndex:inIndex];
										subSketchOffsets.push_back((uint16_t)offset/2);
										offset += length;
									} else
									{
										[self->_multiAppLogViewController postErrorString: [NSString stringWithFormat:
											@"Unable to open the %@.elf file and/or the elf file is damaged and/or this is not an AVR device.", inSketchRec[kNameKey]]];
										*outStop = YES;
										success = NO;
									}
									// While the elf file is open, check for required symbols...
									if (success)
									{
										switch (inIndex)
										{
											case 0:	// Forwarder Sketch
											{
												const SSymbolTblEntry*	symTableEntry = NULL;
												if (!elfFile.GetSymbolValuePtr("_ZL17currSketchAddress", &symTableEntry) ||
													!elfFile.GetSymbolValuePtr("restart", &symTableEntry))
												{
													[self->_multiAppLogViewController postErrorString: [NSString stringWithFormat:
														@"The first sketch in the list must be the Forwarder sketch, %@ is not a Forwarder.\n"
															"One or more expected symbols not found: currSketchAddress, restart" , inSketchRec[kNameKey]]];
													*outStop = YES;
													success = NO;
												} else
												{
													forwarderDataSize = elfFile.GetDataSize();
												}
												break;
											}
											case 1:	// Selector Sketch
											{
												const SSymbolTblEntry*	symTableEntry = NULL;
												// was _ZL23forwarderCurrSketchAddr pre Catalina, Arduino 1.8.10
												// was _ZL16subSketchAddress
												if (!elfFile.GetSymbolValuePtr("forwarderCurrSketchAddr", &symTableEntry) ||
													!elfFile.GetSymbolValuePtr("forwarderRestartPlaceholder", &symTableEntry) ||
													!elfFile.GetSymbolValuePtr("subSketchAddress", &symTableEntry))
												{
													[self->_multiAppLogViewController postErrorString: [NSString stringWithFormat:
														@"The second sketch in the list must be the Selector sketch, %@ is not a Selector.\n"
															"One or more expected symbols not found: forwarderCurrSketchAddr, subSketchAddress, forwarderRestartPlaceholder" , inSketchRec[kNameKey]]];
													*outStop = YES;
													success = NO;
												} else if (symTableEntry->size < (sketches.count > 2 ? ((sketches.count -2)*2) : 0))
												{
													[self->_multiAppLogViewController postErrorString: [NSString stringWithFormat:
														@"Increase the capacity of the Selector sketch array subSketchAddress to %ld.", (sketches.count -2)]];
													*outStop = YES;
													success = NO;
												}/* else
												{
													uint8_t* dataPtr = elfFile.GetSymbolValuePtr("forwarderRestartPlaceholder", &symTableEntry);
													fprintf(stderr, "%hX\n", *(uint16_t*)&dataPtr[2]);
												}*/
												break;
											}
										}
									}
								}];
							if (success)
							{
								/*
								*	If the forwarder sketch doesn't currently contain all
								*	of the ISRs used by all of the sketches THEN
								*	tell the user to add a FORWARD_ISR macro for each
								*	missing ISR to the forwarder sketch.
								*/
								if (!reqSketchVectors.Diff(forwarderVectors))
								{
									NSString*	forwarderVecMacros = [NSString string];
									IndexVecIterator indVedItr(&reqSketchVectors);
									for (size_t index = indVedItr.Current();
											index != IndexVecIterator::end;
													index = indVedItr.Next())
									{
										forwarderVecMacros = [NSString stringWithFormat:@"%@\nFORWARD_ISR(%ld)", forwarderVecMacros, index];
									}
									success = NO;
									[_multiAppLogViewController postErrorString: [NSString stringWithFormat:
										@"The following macros need to be added to the forwarder sketch: \n%@\n\n"
										"Without these macros not all of the ISRs implemented in the "
										"sub sketches will be forwarded.", forwarderVecMacros]];
								}
							}
						}
					}
					/*
					*	Generally there should only be a single common device name, but there
					*	are cases where two devices are compatible, such as the atmega328p and
					*	the atmega328pb.  By compiling some of the sketches for the atmega328p
					*	when run on the atmega328pb will save some space.  (The atmega328pb's
					*	vector table is larger.)
					*/
					if (success)
					{
						NSURL* avrFolderURL = [_arduinoURL URLByAppendingPathComponent:@"Contents/Java/hardware/tools/avr/lib/gcc/avr"];
						success = [self createModSpecsForDevices:sketches avrFolderURL:avrFolderURL dataOffset:forwarderDataSize];
						/*
						*	If success, the modified specs files exist.
						*	Now modify and/or regenerate the elf, hex, and eep files.
						*/
						if (success)
						{
							uint32_t currSketchAddress = 0;
							uint32_t timer0_millisAddress = 0;
							uint32_t timer0_fractAddress = 0;
							uint32_t timer0_overflow_countAddress = 0;
							uint32_t restartAddress = 0;
							NSUInteger	sketchCount = sketches.count;
							NSMutableDictionary* sketchRec;
							for (NSUInteger sketchIndex = 0; success && sketchIndex < sketchCount; sketchIndex++)
							{
								sketchRec = [sketches objectAtIndex:sketchIndex];
								BoardsConfigFile	sketchConfigFile;
								BoardsConfigFile*	configFile = outConfigFiles.GetConfigForFQBN(((NSString*)[sketchRec objectForKey:kFQBNKey]).UTF8String);
								configFile = [self finalizeConfigFor:sketchRec ioConfigFile:configFile];
								if (configFile)
								{
									/*
									*	If not the Forwarder sketch...
									*/
									if (sketchIndex)
									{
										// Create a new elf file with offset .text and .data
										success = [self offsetTextAndDataFor:sketchRec ioConfigFile:configFile];
									}
								} else
								{
									success = NO;
								}
								
								if (success)
								{
									std::string	elfPath([MainWindowController elfPathFor:sketchRec forKey:kTempCopyURLKey]);
									AVRElfFile	elfFile;
									success = elfFile.ReadFile(elfPath.c_str());
									if (success)
									{
										const SSymbolTblEntry*	symTableEntry = NULL;
										uint16_t*  elfOffset;
										/*
										*	If this is the Forwarder sketch THEN
										*	get the .text/flash addresses of the
										*	currSketchAddress and reset symbols.
										*/
										if (sketchIndex == 0)
										{
											// Set the address of the Selector sketch
											// Note that symbol validation/existance
											// has already taken place earlier. Any testing for NULL
											// below is just a sanity check.
											elfOffset = (uint16_t*)elfFile.GetSymbolValuePtr("_ZL17currSketchAddress", &symTableEntry);
											currSketchAddress = symTableEntry->value;
											if (elfOffset)
											{
												// note that the currSketchAddress is fed to an ijmp instruction so
												// it needs to be divided by 2.
												*elfOffset = ((NSNumber*)[sketchRec objectForKey:kLengthKey]).unsignedShortValue/2;
											} else
											{
												success = NO;
											}
											if (success)
											{
												symTableEntry = NULL;
												elfFile.GetSymbolValuePtr("timer0_millis", &symTableEntry);
												if (symTableEntry)
												{
													timer0_millisAddress = symTableEntry->value;
													elfFile.GetSymbolValuePtr("timer0_fract", &symTableEntry);
													if (symTableEntry)
													{
														timer0_fractAddress = symTableEntry->value;
														elfFile.GetSymbolValuePtr("timer0_overflow_count", &symTableEntry);
														if (symTableEntry)
														{
															timer0_overflow_countAddress = symTableEntry->value;
														}
													}
												}
												success = symTableEntry != NULL;
											}
											success = success && elfFile.GetSymbolValuePtr("restart", &symTableEntry) != NULL;
											restartAddress = symTableEntry->value;
											// Write the edited elf file
											success = success && elfFile.WriteFile(elfPath.c_str());
										} else
										{
											if (sketchIndex == 1)
											{
												elfOffset = (uint16_t*)elfFile.GetSymbolValuePtr("forwarderCurrSketchAddr", &symTableEntry);
												if (elfOffset)
												{
													*elfOffset = currSketchAddress;
												} else
												{
													success = NO;
												}
												elfOffset = (uint16_t*)elfFile.GetSymbolValuePtr("subSketchAddress", &symTableEntry);
												if (elfOffset &&
													subSketchOffsets.size() > 2 &&
													symTableEntry->size >= ((subSketchOffsets.size()-2)*2))
												{
													Uint16Vec::const_iterator itr = subSketchOffsets.begin()+2;
													Uint16Vec::const_iterator itrEnd = subSketchOffsets.end();
													for (; itr != itrEnd; itr++)
													{
														*(elfOffset++) = *itr;
													}
												} else
												{
													[self->_multiAppLogViewController postErrorString: @"There are no sub sketches."];
													success = NO;
												}
												elfOffset = (uint16_t*)elfFile.GetSymbolValuePtr("forwarderRestartPlaceholder", &symTableEntry);
												if (elfOffset)
												{
													*(uint32_t*)elfOffset = AVRElfFile::JmpInstructionFor(restartAddress);
												} else
												{
													success = NO;
												}
											}
											if (success)
											{
												// No error checking.  It may not be used.
												elfFile.ReplaceAddress("timer0_millis", timer0_millisAddress);
												elfFile.ReplaceAddress("timer0_fract", timer0_fractAddress);
												elfFile.ReplaceAddress("timer0_overflow_count", timer0_overflow_countAddress);
											}
											// Write the edited elf file
											success = success && elfFile.WriteFile(elfPath.c_str());
										}
									}
									if (!success)
									{
										[self->_multiAppLogViewController postErrorString: [NSString stringWithFormat:
												@"Elf file not created: %s.", elfPath.c_str()]];
									}
								}
	#ifdef AVR_OBJ_DUMP
								if (success)
								{
									success = [self runShellForRecipe:"recipe.elfdump.pattern" configFile:configFile];
								}
	#endif
								if (success)
								{
									success = [self runShellForRecipe:"recipe.objcopy.eep.pattern" configFile:configFile] &&
											[self runShellForRecipe:"recipe.objcopy.hex.pattern" configFile:configFile];
									if (success)
									{
										[self->_multiAppLogViewController postInfoString: [NSString stringWithFormat:
											@"hex and eep files created for: %@.", [sketchRec objectForKey:kNameKey]]];
									} else
									{
											[self->_multiAppLogViewController postErrorString: [NSString stringWithFormat:
												@"hex and eep files not created for: %@.", [sketchRec objectForKey:kNameKey]]];
										break;
									}
								}
							}
							if (success)
							{
								std::string	flashMaxSize;
								outConfigFiles.GetPrimaryConfig()->RawValueForKey("upload.maximum_size", flashMaxSize);
								summaryTextField.stringValue = [NSString stringWithFormat:@"Flash Used: %d of %s", offset, flashMaxSize.c_str()];
								success = atoi(flashMaxSize.c_str()) >= offset;
								if (success)
								{
									// At this point the hex and epp files have been created.
									// Concatenate the hex and epp files.
									success = [self concatenateHexFiles:sketches];
									if (success)
									{
										[_multiAppLogViewController postInfoString: @"Combined hex and eep created."];
									} else
									{
										[_multiAppLogViewController postErrorString: @"Combined hex and eep files not created."];
									}
								} else
								{
									[_multiAppLogViewController postErrorString: [NSString stringWithFormat:
										@"Flash size exceeded, should be less than: %s.", flashMaxSize.c_str()]];
								}
							}
						}
					}
#if SANDBOX_ENABLED
					[_packagesFolderURL stopAccessingSecurityScopedResource];
				}
				[_arduinoURL stopAccessingSecurityScopedResource];
			}
			[_tempFolderURL stopAccessingSecurityScopedResource];
#endif
		} else if (success) // if success but no access to temp folder
		{
			success = NO;
			[_multiAppLogViewController postErrorString: @"Unable to access the temporary items folder (sandbox issue.)"];
		}
	}

	return(success);
}

/************************** createModSpecsForDevices **************************/
/*
*	Verify the location of the specs folder.  For each unique device a
*	modified version of the specs file needs to be created so that the
*	SRAM/data starting offset can be shifted by the size of the SRAM used
*	by the Forwarder sketch.  A copy of the original is made with a modified
*	-Tdata specs offset by inDataOffset bytes.
*/
- (BOOL)createModSpecsForDevices:(NSMutableArray<NSMutableDictionary*>*)inSketches
			avrFolderURL:(NSURL*)inAVRFolderURL dataOffset:(uint32_t)inDataOffset
{
	BOOL success = YES;
	NSMutableArray<NSString*>* deviceNames = [NSMutableArray arrayWithCapacity:2];
	NSMutableArray<NSString*>* modSpecsNames = [NSMutableArray arrayWithCapacity:2];
	NSURL* modSpecsFileURL = nil;
	// Only expecting one folder in avr but its name changes based on the gcc version...
	NSArray* avrContents = [[NSFileManager defaultManager] contentsOfDirectoryAtURL:inAVRFolderURL includingPropertiesForKeys:nil options:NSDirectoryEnumerationSkipsSubdirectoryDescendants error:nil];
	NSURL* specsFolderURL = avrContents.count >= 1 ? [avrContents objectAtIndex:0] : nil;
	if (specsFolderURL)
	{
		/*
		*	/Contents/Java/hardware/tools/avr/lib/gcc/avr/5.4.0/device-specs/
		*	First locate the gcc/avr folder, then get past the version numbered folder
		*	and then check for the existance of the device-specs folder.
		*/
		specsFolderURL = [specsFolderURL URLByAppendingPathComponent:@"device-specs"];
		if ([[NSFileManager defaultManager] fileExistsAtPath:specsFolderURL.path])
		{
			// For each of the device specs, create a copy containing
			// the modified -Tdata incremented by 2

			NSMutableDictionary* sketchRec;
			NSUInteger	sketchCount = inSketches.count;
			for (NSUInteger sketchIndex = 0; success && sketchIndex < sketchCount; sketchIndex++)
			{
				sketchRec = [inSketches objectAtIndex:sketchIndex];
				NSString*	deviceName = [sketchRec objectForKey:kDeviceNameKey];
				if (![deviceNames containsObject:deviceName])
				{
					[deviceNames addObject:deviceName];

					// Open the specs file and read it's contents into memory...
					NSURL* specsFileURL = [specsFolderURL URLByAppendingPathComponent:[NSString stringWithFormat:@"specs-%@", deviceName]];
					modSpecsFileURL = [specsFolderURL URLByAppendingPathComponent:[NSString stringWithFormat:@"specs-%@mod", deviceName]];
				
					NSString* specsText = [NSString stringWithContentsOfURL:specsFileURL encoding:NSUTF8StringEncoding error:nil];
					if (specsText)
					{
						// Locate the line containing the -Tdata option.
						NSRange tDataRange = [specsText rangeOfString:@"-Tdata"];
						if (tDataRange.location != NSNotFound)
						{
							// Extract the -Tdata option value
							// Assumption: the value starts with the 0x prefix and is a hex number.
							NSRange tDataLineRange = [specsText lineRangeForRange:tDataRange];
							NSRange tDataValueRange = [specsText rangeOfString:@"0x" options:NSLiteralSearch range:tDataLineRange];
							const char*	dataValuePtr = &specsText.UTF8String[tDataValueRange.location + tDataValueRange.length];
							uint32_t	value = 0;
							sscanf(dataValuePtr, "%X", &value);
						
							value += inDataOffset;	// Add the amount of SRAM used by the Forwarder
							char valueStr[10];
							int bytesCreated = sprintf(valueStr, "%X", value);
							// Now verify that there is a "mod" specs file and it contains the correct -Tdata value
							NSString* modSpecsText = [NSString stringWithContentsOfURL:modSpecsFileURL encoding:NSUTF8StringEncoding error:nil];
							if (modSpecsText)
							{
								uint32_t	modValue = 0;
								NSRange modTDataRange = [modSpecsText rangeOfString:@"-Tdata"];
								if (modTDataRange.location != NSNotFound)
								{
									// Extract the modified -Tdata option value
									NSRange modTDataLineRange = [modSpecsText lineRangeForRange:modTDataRange];
									NSRange modTDataValueRange = [modSpecsText rangeOfString:@"0x" options:NSLiteralSearch range:modTDataLineRange];
									const char*	modTDataValuePtr = &modSpecsText.UTF8String[modTDataValueRange.location + modTDataValueRange.length];
									sscanf(modTDataValuePtr, "%X", &modValue);
								}
								// If the modified specs file is out of sync THEN
								// flag it to be created.
								if (modValue != value)
								{
									modSpecsText = nil;
								}
							}
							// If a modified specs file needs to be created...
							if (!modSpecsText)
							{
								modSpecsText = [specsText stringByReplacingCharactersInRange:
									NSMakeRange(tDataValueRange.location + tDataValueRange.length, bytesCreated)
										withString:[NSString stringWithUTF8String:valueStr]];
#if SANDBOX_ENABLED
								NSURL* tempModSpecsFileURL = [_appTempFolderURL URLByAppendingPathComponent:[NSString stringWithFormat:@"specs-%@mod", deviceName]];
#else
								NSURL* tempModSpecsFileURL = modSpecsFileURL;
#endif
								NSError*	error;
								[modSpecsText writeToURL:tempModSpecsFileURL atomically:NO encoding:NSUTF8StringEncoding error:&error];
								if (!error)
								{
									[modSpecsNames addObject:[tempModSpecsFileURL.path lastPathComponent]];
								} else
								{
									NSLog(@"%@", error);
								}
#if SANDBOX_ENABLED
								success = NO;
#else
								success = error == nil;
#endif
							}
						}
					}
				}
			}
			
			if (modSpecsNames.count)
			{
				__block NSString*	modSpecsNamesStr = [NSString string];
				[modSpecsNames enumerateObjectsUsingBlock:^(NSString * _Nonnull inSpecsName, NSUInteger inINdex, BOOL * outStop)
				{
					modSpecsNamesStr = [NSString stringWithFormat:@"%@%@\n", modSpecsNamesStr, inSpecsName];
				}];

#if SANDBOX_ENABLED
				[_multiAppLogViewController postErrorString: [NSString stringWithFormat:
					@"The following specs file(s) need to be copied from the temporary folder "
					"to the Arduino specs folder because they either don't exist or need to "
					"be updated. Due to sandbox issues, this application "
					"can't create files within an application bundle.  Please move "
					"the following file(s) to the Arduino specs folder:\n\n%@\n\n"
					"If the from/to folders did not open automatically, they are:\n\n"
					"From: %@\n\n To: %@\n", modSpecsNamesStr, _appTempFolderURL.path,
					specsFolderURL.path]];
				[[NSWorkspace sharedWorkspace] openURL:specsFolderURL];
				[[NSWorkspace sharedWorkspace] openURL:_tempFolderURL];
#else
				[_multiAppLogViewController postWarningString: [NSString stringWithFormat:
					@"The following specs file(s) have been created in the "
					"Arduino specs folder:\n\n%@\n"
					"In folder: %@.\n", modSpecsNamesStr, specsFolderURL.path]];
				// Both of the below work.  The first only opens the folder,
				// the second opens the folder and selects the last added mod file.
				//[[NSWorkspace sharedWorkspace] openURL: specsFolderURL];
				//[[NSWorkspace sharedWorkspace] selectFile:modSpecsFileURL.path inFileViewerRootedAtPath:@""];
#endif
			}
		}
	}
	return(success);
}

/**************************** concatenateHexFiles *****************************/
/*
*	Contatenates both the .hex and .eep files into a single .hex and .epp file
*	stored in the application's temporary folder as combined.hex and combined.eep
*	NOTE: For epp files, no checks for duplicate memory addresses are made.
*	For hex files, there will be no overlap because the .text section has been
*	shifted.
*/
-(BOOL)concatenateHexFiles:(NSMutableArray<NSMutableDictionary*>*) inSketches
{
	BOOL	success = YES;
	NSUInteger	sketchCount = inSketches.count;
	NSMutableDictionary*	sketchRec;
	for (NSUInteger i = 0; success && i < 2; i++)
	{
		NSString*	extension = i == 0 ? @"hex":@"eep";
		fpos_t endOfFileLen = 0;	// To determine the line endings used.
		std::string	combinedBuffer;
		for (NSUInteger sketchIndex = 0; success && sketchIndex < sketchCount; sketchIndex++)
		{
			sketchRec = [inSketches objectAtIndex:sketchIndex];
			std::string srcPath([((NSURL*)[sketchRec objectForKey:kTempCopyURLKey]) URLByAppendingPathComponent:[sketchRec[kNameKey] stringByAppendingPathExtension:extension]].path.UTF8String);
			FILE*    srcFile = fopen(srcPath.c_str(), "rb");
			if (srcFile)
			{
				fseek(srcFile, 0, SEEK_END);
				fpos_t fileSize = 0;
				fgetpos(srcFile, &fileSize);
				rewind(srcFile);
				char* content = new char[fileSize];
				success = fread(content, 1, fileSize, srcFile) == fileSize;
				fclose(srcFile);
				if (success)
				{
					// For each hex file there's an end-of-file record and
					// possibly a start-segment-address record.
					// Inspect the last two records in the file and discard
					// them if found.
					char*	contentPtr = &content[fileSize];
					for (contentPtr--; contentPtr >= content; contentPtr--)
					{
						if (*contentPtr != ':')
						{
							continue;
						} else if (contentPtr[8] == '1')
						{
							endOfFileLen = fileSize;
							fileSize = contentPtr-content;
							endOfFileLen -= fileSize;
							continue;
						} else if (contentPtr[8] == '3')
						{
							fileSize = contentPtr-content;
						}
						break;
					}
					combinedBuffer.append(content, fileSize);
				}
				delete [] content;
			}
		}
		if (success)
		{
			combinedBuffer.append(endOfFileLen == 13 ? ":00000001FF\r\n" : ":00000001FF\n");
			NSURL*	dstFileURL = [_appTempFolderURL URLByAppendingPathComponent:[NSString stringWithFormat:@"combined.%@", extension]];
			FILE*    dstFile = fopen(dstFileURL.path.UTF8String, "wb");
			if (dstFile)
			{
				success = fwrite(combinedBuffer.c_str(), 1, combinedBuffer.size(), dstFile) == combinedBuffer.size();
				fclose(dstFile);
			}
		}
	}
	return(success);
}

/************************** initializeFQBNConfigFor ***************************/
/*
*	Using the fully qualified board name (FQBN), attempt to locate and initialize the
*	corresponding key values specific to this FQBN in the located boards.txt.
*	This creates a common/general configuration for the specific FQBN associated
*	with inSketchRec.  On subsequent calls, if a configuration for the FQBN has
*	already been created, the routine returns immediately.
*	When the configuration needs to be used for the specific sketch,
*	finalizeConfigFor is called.
*/
- (BoardsConfigFile*)initializeFQBNConfigFor:(NSDictionary*)inSketchRec configFile:(BoardsConfigFiles&)ioConfigFiles
{
	BoardsConfigFile*	configFile = NULL;
	NSURL*	tempURL = (NSURL*)[inSketchRec objectForKey:kTempCopyURLKey];
	NSString*	sketchTempPath = tempURL.path;
	NSString*	sketchName = [inSketchRec objectForKey:kNameKey];
	FileInputBuffer		jsonFileInput([sketchTempPath stringByAppendingPathComponent:@"build.options.json"].UTF8String);
	JSONObject*			json = (JSONObject*)IJSONElement::Create(jsonFileInput);
	if (json &&
		json->GetType() == IJSONElement::eObject)
	{
		JSONString* customBuildProperties = (JSONString*)json->GetElement("customBuildProperties", IJSONElement::eString);
		JSONString* fqbn = (JSONString*)json->GetElement("fqbn", IJSONElement::eString);
		if (fqbn)
		{
			configFile = ioConfigFiles.GetConfigForFQBN(fqbn->GetString());
			if (!configFile)
			{
				configFile = new BoardsConfigFile(fqbn->GetString());
				ioConfigFiles.AdoptBoardsConfigFile(configFile);	// ioConfigFiles adopts/takes ownership of configFile
				//inConfigFile.SetFQBNFromString();
				/*
				*	Look through the hardware folders for the Boards.txt and Platform.txt for this FQBN.
				*/
				NSURL*	boardsTxtURL = nil;
				NSURL*	platformTxtURL = nil;
				NSString*	architecture = [NSString stringWithUTF8String:configFile->GetArchitecture().c_str()];
				JSONString* hardwareFolders = (JSONString*)json->GetElement("hardwareFolders", IJSONElement::eString);
				if (hardwareFolders)
				{
					StringInputBuffer	inputBuffer(hardwareFolders->GetString());
					std::string		hardwarePath;
					bool	morePaths = false;
					do
					{
						morePaths = inputBuffer.ReadTillChar(',', false, hardwarePath);
						inputBuffer++;
						hardwarePath += '/';
						hardwarePath.append(configFile->GetPackage());
						NSString*	packagePath = [NSString stringWithUTF8String:hardwarePath.c_str()];
						if ([[NSFileManager defaultManager] fileExistsAtPath:packagePath])
						{
							NSURL*	packageURL = [NSURL fileURLWithPath:packagePath isDirectory:YES];
							NSDirectoryEnumerator* directoryEnumerator =
								[[NSFileManager defaultManager] enumeratorAtURL:packageURL
									includingPropertiesForKeys:@[NSURLNameKey, NSURLIsDirectoryKey]
										options:NSDirectoryEnumerationSkipsHiddenFiles
											errorHandler:nil];
							NSURL*	architectureURL = nil;
							for (NSURL* fileURL in directoryEnumerator)
							{
								NSNumber *isDirectory = nil;
								[fileURL getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];

								if ([isDirectory boolValue])
								{
									NSString* name = nil;
									[fileURL getResourceValue:&name forKey:NSURLNameKey error:nil];

									if ([name isEqualToString:architecture])
									{
										architectureURL = fileURL;
										break;
									}
								}
							}
							if (architectureURL)
							{
								directoryEnumerator =
									[[NSFileManager defaultManager] enumeratorAtURL:architectureURL
										includingPropertiesForKeys:@[NSURLNameKey]
											options:NSDirectoryEnumerationSkipsHiddenFiles
												errorHandler:nil];
								for (NSURL* fileURL in directoryEnumerator)
								{
									NSString* name = nil;
									[fileURL getResourceValue:&name forKey:NSURLNameKey error:nil];
									if ([name isEqualToString:@"boards.txt"])
									{
										boardsTxtURL = fileURL;
										if (platformTxtURL)break;
									} else if ([name isEqualToString:@"platform.txt"])
									{
										platformTxtURL = fileURL;
										NSString* runtimePlatformDir = [fileURL.path stringByDeletingLastPathComponent];
										configFile->InsertKeyValue("runtime.platform.path", runtimePlatformDir.UTF8String);
										if (boardsTxtURL)break;
									}
								}
							}
							break;
						}
						hardwarePath.clear();
					} while(morePaths);
				}
				if (boardsTxtURL && platformTxtURL)
				{
					if (configFile->ReadFile(platformTxtURL.path.UTF8String, false) &&
							configFile->ReadFile(boardsTxtURL.path.UTF8String, true))
					{
						if (customBuildProperties)
						{
							configFile->ReadDelimitedKeyValuesFromString(customBuildProperties->GetString());
						}
						/*
						*	If the customBuildProperties didn't exist (very rare)
						*	or customBuildProperties doesn't contain the expected
						*	tools/avr keys THEN
						*	attempt to add them using the toolsFolders object.
						*/
						std::string value;
						if (!configFile->RawValueForKey("runtime.tools.avr-gcc.path", value))
						{
							JSONString* toolsFolders = (JSONString*)json->GetElement("toolsFolders", IJSONElement::eString);
							if (toolsFolders)
							{
								StringInputBuffer	inputBuffer(toolsFolders->GetString());
								for (uint8_t thisChar = inputBuffer.CurrChar(); thisChar; thisChar = inputBuffer.CurrChar())
								{
									inputBuffer.ReadTillChar(',', false, value);
									if (value.compare(value.length()-3, 3, "avr"))
									{
										value.clear();
										inputBuffer.NextChar();	// Skip the Delimiter
										continue;
									}
									configFile->InsertKeyValue("runtime.tools.avr-gcc.path", value);
									configFile->InsertKeyValue("runtime.tools.avrdude.path", value);
									break;
								}
							}
						}
						// Uncomment to dump the tree
						/*{
							std::string dumpString;
							configFile->GetRootObject()->Write(0, dumpString);
							fprintf(stderr, "\n\n%s\n", dumpString.c_str());
						}*/
					} else
					{
						configFile = NULL;
						[_multiAppLogViewController postErrorString: [NSString stringWithFormat:@"Unable to load boards.txt and/or platform.txt for %@", sketchName]];
					}
				} else
				{
					[_multiAppLogViewController postErrorString: [NSString stringWithFormat:@"Unable to locate boards.txt and/or platform.txt for %@", sketchName]];
				}
	#ifdef AVR_OBJ_DUMP
				if (configFile)
				{
					// For debugging
					configFile->InsertKeyValue("recipe.elfdump.pattern",
						"\"{compiler.path}avr-objdump\" -h -S -d -t -j .data -j .text -j .bss "
						"\"{build.path}/{build.project_name}.elf\" > "
						"\"{build.path}/{build.project_name}M.elf.txt\"");
				}
	#endif
			}
		}
	}
	return(configFile);
}

/***************************** finalizeConfigFor ******************************/
/*
*	This is called after initializeFQBNConfigFor, when the configuration needs
*	to be used for the specific sketch.
*/
- (BoardsConfigFile*)finalizeConfigFor:(NSDictionary*)inSketchRec ioConfigFile:(BoardsConfigFile*)ioConfigFile
{
	NSString*	inoName = [inSketchRec objectForKey:kNameKey];
	if (ioConfigFile)
	{
		NSURL*	tempURL = (NSURL*)[inSketchRec objectForKey:kTempCopyURLKey];
		NSString*	sketchTempPath = tempURL.path;
		// Keys normally added by Arduino
		{
			ioConfigFile->InsertKeyValue("build.path", sketchTempPath.UTF8String);
			ioConfigFile->InsertKeyValue("build.project_name", inoName.UTF8String);
			// object files referenced by the root object_files key
			{
				NSString*	cppObjPath = [[[sketchTempPath stringByAppendingPathComponent:@"sketch"] stringByAppendingPathComponent:inoName] stringByAppendingPathExtension:@"cpp.o"];
				/*
				*	Iterate the library folder and collect any .o file paths.
				*/
				NSDirectoryEnumerator* directoryEnumerator =
					[[NSFileManager defaultManager] enumeratorAtURL:[tempURL URLByAppendingPathComponent:@"libraries"]
						includingPropertiesForKeys:nil
							options:NSDirectoryEnumerationSkipsHiddenFiles
								errorHandler:nil];
				for (NSURL* fileURL in directoryEnumerator)
				{
					if ([fileURL.path.pathExtension isEqualToString:@"o"])
					{
						cppObjPath = [NSString stringWithFormat:@"%@ %@", cppObjPath, fileURL.path];
					}
				}
				ioConfigFile->InsertKeyValue("object_files", cppObjPath.UTF8String);
			}
			/*
			*	See if there's a cached core.a file with a mangled FQBN prefix.
			*	If there is then use it. If not, look for the core.a in the core
			*	folder of the sketch temp folder.
			*/
			{
				NSString* cachedCoreNamePrefix = [NSString stringWithUTF8String:ioConfigFile->GetCoreFQBNPrefix().c_str()];
				NSArray<NSURL*>* contentsOfDirectory = [[NSFileManager defaultManager] contentsOfDirectoryAtURL:
					[_appTempFolderURL URLByAppendingPathComponent:@"cache/core"] includingPropertiesForKeys:nil
					options:NSDirectoryEnumerationSkipsHiddenFiles error:nil];
				bool success = false;
				for (NSUInteger fCount = contentsOfDirectory.count; fCount; )
				{
					fCount--;
					if ([[contentsOfDirectory objectAtIndex:fCount].path.lastPathComponent hasPrefix:cachedCoreNamePrefix])
					{
						ioConfigFile->InsertKeyValue("archive_file", [@"/../cache/core/" stringByAppendingPathComponent:[contentsOfDirectory objectAtIndex:fCount].path.lastPathComponent].UTF8String);
						success = true;
						break;
					}
				}
				if (!success)
				{
					success = [[NSFileManager defaultManager] fileExistsAtPath:[sketchTempPath stringByAppendingPathComponent:@"core/core.a"]];
					if (success)
					{
						ioConfigFile->InsertKeyValue("archive_file", "core/core.a");
					/*
					*	FQBN's that are really long only contain the hash value.
					*	I wasn't able to figure out what they're using, probably
					*	some one of the SHA family.  Even knowing that you'd still
					*	need to know what is being fed to to the function.
					*
					*	If there's only one core in the folder THEN
					*	assume it's the one.
					*/
					} else if (contentsOfDirectory.count == 1)
					{
						success = YES;
						ioConfigFile->InsertKeyValue("archive_file", [@"/../cache/core/" stringByAppendingPathComponent:[contentsOfDirectory objectAtIndex:0].path.lastPathComponent].UTF8String);
					} else
					{
						ioConfigFile = NULL;
						[_multiAppLogViewController postErrorString: [NSString stringWithFormat:@"Unable to locate core.a or a cached core for %@", inoName]];
					}
				}
			}
		}
	}
	// Uncomment to dump the tree
	/*if (ioConfigFile)
	{
		std::string dumpString;
		ioConfigFile->GetRootObject()->Write(0, dumpString);
		fprintf(stderr, "\n\n%s\n", dumpString.c_str());
	}*/
	return(ioConfigFile);
}

/*************************** offsetTextAndDataFor *****************************/
/*
*	Sets the flash/.text and SRAM/.data starting addresses.
*	The flash starting address, which is normally 0, is offset by the value of
*	start below.  The SRAM starting address is normally the size of the
*	registers, but is shifted by the size of SRAM used by the Forwarder sketch
*	(forwarderDataSize bytes).  This is done by using an edited specs-xxx file
*	for this device derived from build.mcu (changed to specs-{xxxmod} where xxx
*	is the device name).
*/
- (BOOL)offsetTextAndDataFor:(NSDictionary*)inSketchRec ioConfigFile:(BoardsConfigFile*)ioConfigFile
{
	BOOL success = NO;
	if (ioConfigFile)
	{
		std::string	deviceName;
		std::string	compilerCElfFlags;
		ioConfigFile->RawValueForKey("build.mcu", deviceName);
		ioConfigFile->RawValueForKey("compiler.c.elf.flags", compilerCElfFlags);
		{
			uint32_t	start = ((NSNumber*)[inSketchRec objectForKey:kStartKey]).unsignedIntValue;
			std::string modCompilerCElfFlags;
			modCompilerCElfFlags.assign(compilerCElfFlags);
			char dotTextStart[251];
			snprintf(dotTextStart, 250, " -Wl,--section-start=.text=0x%X", start);
			modCompilerCElfFlags.append(dotTextStart);
			ioConfigFile->InsertKeyValue("compiler.c.elf.flags", modCompilerCElfFlags);
			// The name of the mcu determines which specs-{build.mcu} is used.
			std::string modDeviceName(deviceName);
			modDeviceName.append("mod");
			ioConfigFile->InsertKeyValue("build.mcu", modDeviceName);
		}
		success = [self runShellForRecipe:"recipe.c.combine.pattern" configFile:ioConfigFile];
		// reset the modified values, the configFile is common
		ioConfigFile->InsertKeyValue("build.mcu", deviceName);
		ioConfigFile->InsertKeyValue("compiler.c.elf.flags", compilerCElfFlags);
		if (success)
		{
			[self->_multiAppLogViewController postInfoString: [NSString stringWithFormat:
				@"Elf file created for: %@.", [inSketchRec objectForKey:kNameKey]]];
		}
	}
	return(success);
}

/**************************** verifyAppTempFolder *****************************/
- (BOOL)verifyAppTempFolder
{
	BOOL	success = YES;
	#if SANDBOX_ENABLED
	_appTempFolderURL = [[NSFileManager defaultManager] temporaryDirectory];
	#else
	_appTempFolderURL = [[[NSFileManager defaultManager] temporaryDirectory]
							URLByAppendingPathComponent:[NSBundle.mainBundle.executablePath lastPathComponent]];
	if (![[NSFileManager defaultManager] fileExistsAtPath:_appTempFolderURL.path])
	{
		NSError*	error = nil;
		success = [[NSFileManager defaultManager] createDirectoryAtURL:_appTempFolderURL withIntermediateDirectories:NO attributes:nil error:&error];
		if (!success)
		{
			[self->_multiAppLogViewController postErrorString: [NSString stringWithFormat:
				@"Unable to create application temporary folder: %@.", error]];
		}
	}
	#endif
	return(success);
}

/************************** copyArduinoTempFolderFor **************************/
/*
*	One more sandbox hoop to jump through... NSTask apps don't inherit
*	the entitlements of this app so they can't create files outside of
*	the defined sandbox when this app launched.  This means that bookmarks,
*	which are entitlements added after launch, aren't observed for the NSTask.
*	To get around this the Arduino temporary folders are copied to this
*	app's temporary folder within the sandbox.
*/
- (BOOL)copyArduinoTempFolderFor:(NSDictionary*)inSketchRec
{
	NSURL*	srcTempURL = (NSURL*)[inSketchRec objectForKey:kTempURLKey];
	NSString*	name = (NSString*)[inSketchRec objectForKey:kNameKey];
	NSURL*	tempCopyURL = [_appTempFolderURL URLByAppendingPathComponent:[name stringByDeletingPathExtension]];
	NSError* error = nil;
	[[NSFileManager defaultManager] copyItemAtURL:srcTempURL toURL:tempCopyURL error:&error];
	if (error)
	{
		[_multiAppLogViewController postErrorString: [NSString stringWithFormat:@"Unable to copy the Arduino temp folder for %@\n%@\n", [inSketchRec objectForKey:kNameKey], error]];
	} else
	{
		[inSketchRec setValue:tempCopyURL forKey:kTempCopyURLKey];
	}
	return(error == nil);
}

/***************************** runShellForRecipe ******************************/
- (BOOL)runShellForRecipe:(const char*)inRecipeKey configFile:(BoardsConfigFile*)inConfigFile
{
	BOOL		success = NO;
	std::string value;
	uint32_t keysNotFound = 0;
	inConfigFile->ValueForKey(inRecipeKey, value, keysNotFound);
	//fprintf(stderr, "keysNotFound = %d\n", keysNotFound);
	//fprintf(stderr, "%s\n", value.c_str());
	std::string escapedArguments;
	NSURL*	exeURL = [NSURL fileURLWithPath:@"/bin/bash"];
	NSArray<NSString*>* arguments = @[ @"-c", [NSString stringWithUTF8String:value.c_str()] ];
	NSTask* task = [[NSTask alloc] init];
	task.arguments = arguments;
	//[_multiAppLogViewController postInfoString:[NSString stringWithFormat:@"\"%@\" %@",exeURL.path, arguments]];
	task.executableURL = exeURL;
	__block NSString* taskOutputStr;
	__block NSString* taskErrorStr;
	task.standardOutput = [NSPipe pipe];
	task.standardError = [NSPipe pipe];
	task.terminationHandler = ^(NSTask* task)
	{
		NSData* taskOutput = [[task.standardOutput fileHandleForReading] readDataToEndOfFile];
		taskOutputStr = [[NSString alloc] initWithData:taskOutput encoding:NSUTF8StringEncoding];
		NSData* taskError = [[task.standardError fileHandleForReading] readDataToEndOfFile];
		taskErrorStr = [[NSString alloc] initWithData:taskError encoding:NSUTF8StringEncoding];

		/*if (task.terminationReason == NSTaskTerminationReasonExit && task.terminationStatus == 0)
		{
		}*/
	};
	@try
	{
		NSError *error = nil;
		
		success = [task launchAndReturnError:&error];
		[task waitUntilExit];
		if ([taskOutputStr length])
		{
			[self->_multiAppLogViewController postInfoString: [NSString stringWithFormat:@"%@\n", taskOutputStr]];
		}
		if ([taskErrorStr length])
		{
			success = NO;
			[self->_multiAppLogViewController postErrorString: [NSString stringWithFormat:@"%@\n", taskErrorStr]];
		}
		if (!success)
		{
			NSLog(@"%@", error);
		}
	}
	@catch(NSException*	inException)
	{
		NSLog(@"%@", inException);
	}

	return(![task terminationStatus]);
}
@end
