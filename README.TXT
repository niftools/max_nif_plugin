                        MaxPlugins 3.0.7
                        =================

 
    This plugin currently consists of an exporter, importer and a utility 
     plugin for 3ds max and gmax for read and writing Gamebryo/Netimmerse
     related files.
    
    Report problems and ask questions here:
    
        http://www.niftools.org/forum/viewforum.php?f=6
        
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
    
    Installer:
        The installer will configure the plugins for whichever releases
        you have available on your machine.  It will also optionally configure
        texture paths for various games that we know there settings for.
    
    Copy NifPlugins.dlu, to your 3dsmax\plugins directory.
    Copy MaxNifTools.ini to your 3dsmax\plugcfg directory.
    
    gmax notes:
       NifPlugins.dlu goes in gmax\plugin folder
       MaxNifTools.ini goes in gmax\plugCFG folder
       winmm.dll goes in gmax folder where gmax.exe is located.
         Do not place it in the windows system folder.
        
    Usage
    -----
    
      Collision:
        
        Add bhkRigidBody from Create | Helpers | Niftools to represent
          a rigid body in havok.
          
        Add bhkCollProxy or bhkRigidBody Modifier to represent shapes
          and then link them to the bhkRigidBody.
          
      Exporter:
        Export files using "File | Export | Netimmerse/Gamebryo (*.NIF, *.KF)".

      Importer:
        Import files using "File | Import | Netimmerse/Gamebryo (*.NIF, *.KF)"
  
      Settings:
        Its recommended that you edit the MaxNifTools.ini file to adjust 
        paths and other settings to your machine to make it easier to work with.
    
        1. Remove irrelevant applications from the KnownApplications list
        2. Fix the RootPaths, TexturePaths and TextureSearchPaths in the 
            Applications for your machine.
        

    Additional Notes
    ----------------

    If your collision mesh comes out deformed, go to the utilities tab, 
    choose "Reset XForm" and click "Reset Selected". This should fix it.
    