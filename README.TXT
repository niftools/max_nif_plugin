                        MaxPlugins 0.2.17
                        =================

 
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

    Requirements
    ------------

      Plugins:
        3D Studio Max 5, 6, 7, 8, 9, 2008, 2009
        gmax 1.2

    Installation
    ------------
    
    Copy NifPlugins.dlu, to your 3dsmax\plugins directory.
    Copy MaxNifTools.ini to your 3dsmax\plugcfg directory.
    
    Please remove the following when upgrading to 0.2.6 or later release:
       MaxNifImport.dli, NifExport.dle, NifProps.dlu, NifFurniture.dlu
    
    gmax notes:
       NifPlugins.dlu goes in gmax\plugin folder
       MaxNifTools.ini goes in gmax\plugCFG folder
       nifgmax.exe goes in gmax folder where gmax.exe is located.

       You must start gmax with the nifgmax.exe, NOT the gmax.exe.
    
        
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


    Additional Notes
    ----------------
        
    Set the layer value to "NonCollidable", if you want to be able to walk
     through your objects, while still being able to activate them.

    If your collision mesh comes out deformed, go to the utilities tab, 
    choose "Reset XForm" and click "Reset Selected". This should fix it.
    