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
//  ArduinoAppOpenDelegate.m
//  AVRMultiSketch
//
//  Created by Jon Mackey on 11/3/18.
//  Copyright © 2018 Jon Mackey. All rights reserved.
//

#import "ArduinoAppOpenDelegate.h"
//#import "GameAddViewController.h" @interface ViewController : UIViewController <GameAddViewControllerDelegate> @end//
@implementation ArduinoAppOpenDelegate
#if SANDBOX_ENABLED
NSString *const kArduinoAppURLBMKey = @"arduinoAppURLBM";
#else
NSString *const kArduinoAppPathKey = @"arduinoAppPath";
#endif
NSString *const kArduinoBundleIdentifier = @"cc.arduino.Arduino";

- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url
{
	return([[url.path lastPathComponent] hasPrefix:@"Arduino"]);
}

- (BOOL)panel:(id)sender validateURL:(NSURL *)url error:(NSError * _Nullable *)outError
{
	BOOL success = NO;
	NSURL*	arduinoAppURL = url;
	// The user just selected the app so it's in the sandbox.
	// Verify that the CFBundleIdentifier is cc.arduino.Arduino
	NSURL* infoPListURL = [NSURL fileURLWithPath:[arduinoAppURL.path stringByAppendingPathComponent:@"Contents/Info.plist"] isDirectory:NO];
	NSDictionary*	plist = [NSDictionary dictionaryWithContentsOfURL:infoPListURL];
	if (plist &&
		[(NSString*)[plist objectForKey:@"CFBundleIdentifier"] isEqualToString:kArduinoBundleIdentifier])
	{
		NSError*	error;
#if SANDBOX_ENABLED
		NSData*	arduinoAppURLBM = [arduinoAppURL bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
				includingResourceValuesForKeys:NULL relativeToURL:NULL error:&error];
		[[NSUserDefaults standardUserDefaults] setObject:arduinoAppURLBM forKey:kArduinoAppURLBMKey];
#else
		[[NSUserDefaults standardUserDefaults] setObject:arduinoAppURL.path forKey:kArduinoAppPathKey];
#endif
		if (error)
		{
			NSLog(@"-selectArduinoApp- %@\n", error);
			[[NSUserDefaults standardUserDefaults] removeObjectForKey:kArduinoAppPathKey];
			NSAlert *alert = [[NSAlert alloc] init];
			[alert setMessageText:@"Arduino Application"];
			[alert setInformativeText:@"A valid Arduino application was not selected."];
			[alert addButtonWithTitle:@"OK"];
			[alert setAlertStyle:NSAlertStyleWarning];
			[alert runModal];
		}
		success = !error;
	}
	return(success);
}

@end
