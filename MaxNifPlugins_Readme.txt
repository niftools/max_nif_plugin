                        MaxPlugins 0.1.5
                        ================

 
    This plugin set currently consists of an exporter, importer and a utility 
     plugin. This is a early release, so expect it to be buggy.      
    
    Report problems and ask questions here:
    
        http://niftools.sourceforge.net/forum/viewforum.php?f=6
        
    A tutorial on how to use the exporter and utility plugins can be found here:
    
        http://www.silgrad.com/wbb2/thread.php?threadid=2477    
        
    Enjoy!

    Additional Notes
    ----------------
        
    Set the layer value to "NonCollidable", if you want to be able to walk through 
    your objects, while still being able to activate them.

    If your collision mesh comes out deformed, go to the utilities tab, 
    choose "Reset XForm" and click "Reset Selected". This should fix it.


    Change log
    ----------
    
      0.2
      -----
    o Importer
      - Added Vertex Color modifier support
      - Fixed issue with static animation import
      - Fixed issue with skin vertex weight count import
      - Added support for more material/texture properties
      - Added support for Civilization IV Shader, if installed
        o holds much of the material data in nif compatible form
      
    o Exporter
      - Upgraded to Visual Studio 2005
      - Dropped registry support in favor of INI file.
      - Dropped Official Max6 support because I do not have Max 6 to compile it
        o Try editing the MaxNifTools.ini and setting the MaxSDKVersion to 0x17700d00
      - Fixed issue with importing glossiness setting on textures.
      - Fixed issues with export of vertex color. Alpha map is now exported as part of the normal color map.
      - No longer exports meshes associated with bone or biped nodes.
      - No longer exports extra NiNode when exporting NiTriGeom-based objects (can be reset in ini file)
      - Mass, Restitution(Ellasticity), and Friction now share values with Reactor(Havok)
      - Modified UPB export to actually export the values in the UserPropBuffer not just a fixed list.
      - Added Skin Modifier export
      - Added support for more material/texture properties
      - Added support for Civilization IV Shader, if installed
        o holds much of the material data in nif compatible form
      - Fixed issue with UV map needing to be flipped
      
    o NifProps Utility
      - Mass, Restitution(Ellasticity), and Friction now share values with Reactor(Havok)

      0.1.5
      -----
      
    o Importer
      - Updated with latest version NifLib
        + Fixes stray empty triangles when importing NiTriStrip meshes
    
      0.1.4
      -----
      
    o Importer
      - Fixed UV values when flipping so they align properly in UVW window
      - Fixed issues with DAoC mesh import (in NifLib)
      - Fixed rotation issue with animation import
     
      0.1.3
      -----
      
    o Importer
     - Fixed alignment issues when importing Morrowind Armor nifs
     - Added initial animation support (only for animations internal to nif, no kf file support yet)
     - Fixed numerous issues with bone system (biped is still broken)
     - Fixed issues with skin and doac nifs
      
      0.1.2
      -----
      
    - Introduced the importer
    
    - Fixed collision generation, turned out that Oblivion
      doesn't like NvTriStrip's strips. Thanks to Razorwing
      for discovering the bug and Tanguy Fautré for his
      stripper (http://users.telenet.be/tfautre/softdev/tristripper).
                  
    - Fixed materials with transforms, flipped UV coords and 
      objects without mapping.
    
    Requirements
    ------------

      Exporter and Utility:
        3D Studio Max 6, 7 and 8
    
      Importer
        3d Studio Max 8

    Installation
    ------------
    
    Copy NifExport.dle, NifProps.dlu, MaxNifImport.dli to your 3dsmax\plugins 
      directory.
  
      Copy MaxNifTools.ini to your 3dsmax\plugcfg directory.
        
    Usage
    -----
    
      Exporter and Utility:
      Use the NifProps utility plugin to mark objects as collision shapes and
      set their Havok material and layer properties. Export files using
      "File->Export->Gambryo File (*.nif)".

      Importer:
        Import files using "File->Import->Netimmerse/Gamebryo(*.NIF)"
  
        Its highly recommended that you edit the MaxNifTools.ini file to adjust 
        paths and other settings to your machine to make it easier to work with.
    
        1. Remove irrelevant applications from the KnownApplications list
        2. Fix the RootPaths, TexturePaths and TextureSearchPaths in the 
        Applications for your machine.

       - The ini file has a substitution mechanism to make it easier to just 
         change one location and refer to that path in other areas.
       
       - RootPaths is used to determine if the imported nif is in one of 
         those directories if so it assumes the nif is from that app and uses
         those settings for import.
         
       - TexturePaths is used to test if the relative path in the nif is 
         relative to that directory.
       
       - TextureSearchPaths will be recursively scanned for file names that 
         match the extensions (this can be time consuming so keep it to only 
         relevant directories to you).
 
    Copyright
    ---------
      
    Copyright (c) 2006, NIF File Format Library and Tools. All rights reserved.
    
    Legal
    -----
      
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
