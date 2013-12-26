//
//  NSString+wstring_additions.h
//  GaMBL
//
//  Created by David Ventura on 12/24/13.
//  Copyright (c) 2013 David Ventura. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <string>

@interface NSString (wstring_additions)
+(NSString*) stringWithwstring:(const std::wstring&)string;
-(std::wstring) getwstring;
@end
