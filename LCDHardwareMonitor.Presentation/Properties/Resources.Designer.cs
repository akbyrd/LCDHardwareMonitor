﻿//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version:4.0.30319.42000
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

namespace LCDHardwareMonitor.Presentation.Properties {
    using System;
    
    
    /// <summary>
    ///   A strongly-typed resource class, for looking up localized strings, etc.
    /// </summary>
    // This class was auto-generated by the StronglyTypedResourceBuilder
    // class via a tool like ResGen or Visual Studio.
    // To add or remove a member, edit your .ResX file then rerun ResGen
    // with the /str option, or rebuild your VS project.
    [global::System.CodeDom.Compiler.GeneratedCodeAttribute("System.Resources.Tools.StronglyTypedResourceBuilder", "4.0.0.0")]
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute()]
    [global::System.Runtime.CompilerServices.CompilerGeneratedAttribute()]
    internal class Resources {
        
        private static global::System.Resources.ResourceManager resourceMan;
        
        private static global::System.Globalization.CultureInfo resourceCulture;
        
        [global::System.Diagnostics.CodeAnalysis.SuppressMessageAttribute("Microsoft.Performance", "CA1811:AvoidUncalledPrivateCode")]
        internal Resources() {
        }
        
        /// <summary>
        ///   Returns the cached ResourceManager instance used by this class.
        /// </summary>
        [global::System.ComponentModel.EditorBrowsableAttribute(global::System.ComponentModel.EditorBrowsableState.Advanced)]
        internal static global::System.Resources.ResourceManager ResourceManager {
            get {
                if (object.ReferenceEquals(resourceMan, null)) {
                    global::System.Resources.ResourceManager temp = new global::System.Resources.ResourceManager("LCDHardwareMonitor.Presentation.Properties.Resources", typeof(Resources).Assembly);
                    resourceMan = temp;
                }
                return resourceMan;
            }
        }
        
        /// <summary>
        ///   Overrides the current thread's CurrentUICulture property for all
        ///   resource lookups using this strongly typed resource class.
        /// </summary>
        [global::System.ComponentModel.EditorBrowsableAttribute(global::System.ComponentModel.EditorBrowsableState.Advanced)]
        internal static global::System.Globalization.CultureInfo Culture {
            get {
                return resourceCulture;
            }
            set {
                resourceCulture = value;
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Failed to create plugin directory.
        ///{0}.
        /// </summary>
        internal static string Plugin_DirCreateFail {
            get {
                return ResourceManager.GetString("Plugin_DirCreateFail", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Plugins.
        /// </summary>
        internal static string Plugin_DirName {
            get {
                return ResourceManager.GetString("Plugin_DirName", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Failed to scan for plugins.
        ///{0}.
        /// </summary>
        internal static string Plugin_DirScanFail {
            get {
                return ResourceManager.GetString("Plugin_DirScanFail", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Forwarding drawable registration. Type: {0}.
        /// </summary>
        internal static string Plugin_FwdDrawableReg {
            get {
                return ResourceManager.GetString("Plugin_FwdDrawableReg", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Forwarding view registration. Name: {0}, URI: {1}.
        /// </summary>
        internal static string Plugin_FwdViewReg {
            get {
                return ResourceManager.GetString("Plugin_FwdViewReg", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Failed to load plugins.
        ///{0}.
        /// </summary>
        internal static string Plugin_LoadAllFail {
            get {
                return ResourceManager.GetString("Plugin_LoadAllFail", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Failed to load plugin. Path: {0}
        ///{1}.
        /// </summary>
        internal static string Plugin_LoadSingleFail {
            get {
                return ResourceManager.GetString("Plugin_LoadSingleFail", resourceCulture);
            }
        }
        
        /// <summary>
        ///   Looks up a localized string similar to Ignoring plugin because another plugin with the name &apos;{0}&apos; already exists.
        ///Ignored: {1}
        ///Existing: {3}.
        /// </summary>
        internal static string Plugin_NameConflict {
            get {
                return ResourceManager.GetString("Plugin_NameConflict", resourceCulture);
            }
        }
    }
}