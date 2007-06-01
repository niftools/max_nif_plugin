                        MaxPlugins 0.2.11
                        ================

 
    This plugin currently consists of an exporter, importer and a utility 
     plugin for 3ds max and gmax for read and writing Gamebryo/Netimmerse
     related files.
    
    Report problems and ask questions here:
    
        http://www.niftools.org/forum/viewforum.php?f=6
        
    A tutorial on how to use the exporter and utility plugins can be found here:
    
        http://www.silgrad.com/wbb2/thread.php?threadid=2477    

    A wiki for the 3ds Max plugins is available at:
        
        http://www.niftools.org/wiki/index.php/3ds_Max

    Enjoy!


    Additional Notes
    ----------------
        
    Set the layer value to "NonCollidable", if you want to be able to walk
     through your objects, while still being able to activate them.

    If your collision mesh comes out deformed, go to the utilities tab, 
    choose "Reset XForm" and click "Reset Selected". This should fix it.
    

    Change log
    ----------
      0.2.11 (Experimental Release)
      -----
    o All
      - Please note that this is most an experimental release!
      - Update to latest version of Niflib
      - Introduce gmax importer/exporter
      - Introduce build for max 4.2
      
    o Exporter
      - Introduce bhkRigidBody export for Box
      
      0.2.10
      ----- 
    o All
      - Compile with Max9
    o Exporter
      - Fix naming bug when spliting multi-material meshes
      
      0.2.9
      -----   
    o Exporter
      - Add BSX flags for NIF exports when animation is involved for Oblivion.
      - Worked around what appears to be a bug in the Bezier Scale controller.
      - Change time intervals when exporting MW style animation.
      - Fixed bug with missing reference in Footer in MW exported .KF files.
      - Call routine to apply skin offsets to Skin data (required for MW?)
    
    o Importer
      - Invoke Max Script in Max 5 so that we can import Note Tracks
    
      0.2.8
      -----   
    o Exporter
      - Fix Collapse Transforms on dialog so that it actually triggers collapse
      - Change defaults for textures so Morrowind does not crash in CS
      - Fix export of animation for Morrowind
      - Introduce mechanism in Collapse Transforms so that poorly scaled 
        models export better

    o Importer
      - Fix import of animation for Morrowind
    
      0.2.7
      -----
    o All
      - Add options to disable certain parts of plug via INI
        o Added so people who only want importer can disable exporter.
      - Enhancements from NifLib including support for Freedom Force
      - Max 5 build is available but untested.
    
    o Exporter
      - Fix bug with NiTriShape export which created corrupted files.
      - Add option to Update Tangent Space (for Oblivion)
      - Add option to Collapse Transforms on meshes (mostly a debug aide)
      
        
      0.2.6
      -----
    o All
      - Consolidate code to single plugin: NifPlugins.dlu
      - Started Wiki Documentation Project:
        o http://www.niftools.org/wiki/index.php/3ds_Max
      
    o Exporter
      - Fix bug in Skin Partitioning that would cause max to hang
      - Add links to website and wiki to dialog
      - Tag NIF files with Plugin version in first comment field.
      - Fix export of selected nodes.
      - Support Prn strings for Oblivion and Morrowind when selected
        mesh is bound to bone bug does not have skin modifier.
      - Change export to place all data in NonAccum nodes when available
        Plan to do proper support for NonAccum nodes in the future.
      - Add progress bar updates while exporting
      
    o Importer
      - Add links to website and wiki to dialog
      - Add support for Prn string imports for Oblivion and Morrowind
      
      
      0.2.5
      -----
    o Exporter
      - Rewrote animation export dialogs and procedures to write files
        similar to Civilation exporter.
      - Removed seperate *.kf and merged with *.nif like Civ4
      - Fixed issues with NiStencilProperty (via niflib) for Morrowind
      - Add option to optionally export accum nodes
      - Add options for exporting skin partition in different ways
      - Expose the Auto detect parameter to allow game to be saved
      
    o Importer
      - Add option to ignore root
      - Expose the Time Tag and Key Note params on nif dialog
      - Expose the Auto detect parameter to allow game to be saved
        
      0.2.4
      -----
    o Exporter
      - Add animation export.
      
    o Importer
      - Add option to ignore UPB buffers.  
        * Useful in preventing LOD Bone warnings from the Civ4 Exporter
      - Fixed problem with Animation Note Tracks not being cleared
      - Fixed issue with BSpline animation when too much data was present
        * Specifically the Civ4 Leaderheads now import animation very well
      - Import Animation Priority into user prop buffer
        
    o NifProps Utility
      - Added Animation Priority
      - Removed unused Globals
    
      0.2.3
      -----
    o Exporter
      - Fix issue when exporting a mesh with a skin modifier that had bones
         that were not used.
      - Fix bug with normals introduced in 0.2.2 when fixing previous problem
      - Changed code to scale the mesh so that Reset XForm is not required
      - Added support for Bone LOD code.  Works with Civ4 Bone LOD Manager.
      - Added support for Lights and Cameras
      
    o Importer
      - Alter code to create Camera nodes when name begins with Camera.
      - Initial pass at Lights
      - Fixed issues with skin modifier on Civ4 Units and Leaderheads
      - Added support for Bone LOD code.  Works with Civ4 Bone LOD Manager.
      - Added support for Lights and Cameras
      - Fixed issues with Textures with muliple UV Sets      
    
      0.2.2
      -----
    o Exporter
      - Fix Export of Normal values
      
    o Importer
      - Fix Import of Normal values
      - Change Import of Bones to merge NonAccum nodes with parent
      - Change Import of Animation to merge NonAccum nodes with parent node 
      - Add Option to Clear Animation on Import
      
      0.2.1
      -----
    o Exporter
      - Replace the Tri Stripper with a new version.
        o The old version had some bugs which came when compiled with VS 2005.
      - Fix issues with transforms on exporting with extra nodes and for 
         collision meshes
        
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
      - Fixed issue with importing glossiness setting on textures.
      - Fixed issues with export of vertex color. Alpha map is now exported
         as part of the normal color map.
      - No longer exports meshes associated with bone or biped nodes.
      - No longer exports extra NiNode when exporting NiTriGeom-based objects 
         (can be reset in ini file)
      - Mass, Restitution(Ellasticity), and Friction now share values with 
         Reactor(Havok)
      - Modified UPB export to actually export the values in the UserPropBuffer 
         not just a fixed list.
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
     - Added initial animation support (only for animations internal to nif, 
        no kf file support yet)
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

      Plugins:
        3D Studio Max 4.2, 5, 6, 7, 8 and 9
        gmax 1.2

    Installation
    ------------
    
    Copy NifPlugins.dlu, to your 3dsmax\plugins directory.
    Copy MaxNifTools.ini to your 3dsmax\plugcfg directory.
    
    Please remove the following when upgrading to 0.2.6 or later release:
       MaxNifImport.dli, NifExport.dle, NifProps.dlu, NifFurniture.dlu
    
        
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
      
    Copyright (c) 2007, NIF File Format Library and Tools. All rights reserved.
    
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
