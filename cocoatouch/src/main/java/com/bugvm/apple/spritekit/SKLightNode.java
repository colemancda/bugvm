/*
 * Copyright (C) 2013-2015 RoboVM AB
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
package com.bugvm.apple.spritekit;

/*<imports>*/

import com.bugvm.apple.foundation.NSObject;
import com.bugvm.apple.uikit.UIColor;
import com.bugvm.objc.ObjCRuntime;
import com.bugvm.objc.annotation.NativeClass;
import com.bugvm.objc.annotation.Property;
import com.bugvm.objc.*;
import com.bugvm.rt.*;
import com.bugvm.rt.annotation.*;
import com.bugvm.rt.bro.*;
import com.bugvm.rt.bro.annotation.*;
import com.bugvm.rt.bro.ptr.*;
/*</imports>*/

/*<javadoc>*/
/**
 * @since Available in iOS 8.0 and later.
 */
/*</javadoc>*/
/*<annotations>*/@Library("SpriteKit") @NativeClass/*</annotations>*/
/*<visibility>*/public/*</visibility>*/ class /*<name>*/SKLightNode/*</name>*/ 
    extends /*<extends>*/SKNode/*</extends>*/ 
    /*<implements>*//*</implements>*/ {

    /*<ptr>*/public static class SKLightNodePtr extends Ptr<SKLightNode, SKLightNodePtr> {}/*</ptr>*/
    /*<bind>*/static { ObjCRuntime.bind(SKLightNode.class); }/*</bind>*/
    /*<constants>*//*</constants>*/
    /*<constructors>*/
    public SKLightNode() {}
    protected SKLightNode(NSObject.SkipInit skipInit) { super(skipInit); }
    /*</constructors>*/
    /*<properties>*/
    @Property(selector = "isEnabled")
    public native boolean isEnabled();
    @Property(selector = "setEnabled:")
    public native void setEnabled(boolean v);
    @Property(selector = "lightColor")
    public native UIColor getLightColor();
    @Property(selector = "setLightColor:")
    public native void setLightColor(UIColor v);
    @Property(selector = "ambientColor")
    public native UIColor getAmbientColor();
    @Property(selector = "setAmbientColor:")
    public native void setAmbientColor(UIColor v);
    @Property(selector = "shadowColor")
    public native UIColor getShadowColor();
    @Property(selector = "setShadowColor:")
    public native void setShadowColor(UIColor v);
    @Property(selector = "falloff")
    public native @MachineSizedFloat double getFalloff();
    @Property(selector = "setFalloff:")
    public native void setFalloff(@MachineSizedFloat double v);
    @Property(selector = "categoryBitMask")
    public native int getCategoryBitMask();
    @Property(selector = "setCategoryBitMask:")
    public native void setCategoryBitMask(int v);
    /*</properties>*/
    /*<members>*//*</members>*/
    /*<methods>*/
    
    /*</methods>*/
}