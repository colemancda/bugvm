/*
 * Copyright (C) 2013-2015 Trillian Mobile AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.robovm.apple.audiotoolbox;

/*<imports>*/
import java.io.*;
import java.nio.*;
import java.util.*;
import org.robovm.objc.*;
import org.robovm.objc.annotation.*;
import org.robovm.objc.block.*;
import org.robovm.rt.*;
import org.robovm.rt.bro.*;
import org.robovm.rt.bro.annotation.*;
import org.robovm.rt.bro.ptr.*;
import org.robovm.apple.foundation.*;
import org.robovm.apple.corefoundation.*;
import org.robovm.apple.coregraphics.*;
import org.robovm.apple.opengles.*;
import org.robovm.apple.coreaudio.*;
import org.robovm.apple.coremedia.*;
/*</imports>*/

/*<javadoc>*/

/*</javadoc>*/
/*<annotations>*//*</annotations>*/
/*<visibility>*/public/*</visibility>*/ class /*<name>*/AudioFileRegionList/*</name>*/ 
    extends /*<extends>*/Struct<AudioFileRegionList>/*</extends>*/ 
    /*<implements>*//*</implements>*/ {

    /*<ptr>*/public static class AudioFileRegionListPtr extends Ptr<AudioFileRegionList, AudioFileRegionListPtr> {}/*</ptr>*/
    /*<bind>*/
    /*</bind>*/
    /*<constants>*//*</constants>*/
    /*<constructors>*/
    public AudioFileRegionList() {}
    public AudioFileRegionList(int mSMPTE_TimeType, int mNumberRegions, AudioFileRegion mRegions) {
        this.setMSMPTE_TimeType(mSMPTE_TimeType);
        this.setMNumberRegions(mNumberRegions);
        this.setMRegions(mRegions);
    }
    /*</constructors>*/
    /*<properties>*//*</properties>*/
    /*<members>*/
    @StructMember(0) public native int getMSMPTE_TimeType();
    @StructMember(0) public native AudioFileRegionList setMSMPTE_TimeType(int mSMPTE_TimeType);
    @StructMember(1) public native int getMNumberRegions();
    @StructMember(1) public native AudioFileRegionList setMNumberRegions(int mNumberRegions);
    @StructMember(2) public native @Array({1}) AudioFileRegion getMRegions();
    @StructMember(2) public native AudioFileRegionList setMRegions(@Array({1}) AudioFileRegion mRegions);
    /*</members>*/
    /*<methods>*//*</methods>*/
}
