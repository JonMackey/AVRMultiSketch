//
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
/*
*	HexNumberFormatter
*	
*	Created by Jon Mackey on 11/29/18.
*	Copyright © 2018 Jon Mackey. All rights reserved.
*/


#import "HexNumberFormatter.h"

@implementation HexNumberFormatter
- (NSString *)stringFromNumber:(NSNumber *)number
{
	return(number != nil ? [NSString stringWithFormat:@"%lx", number.unsignedLongValue] : nil);
}
@end
