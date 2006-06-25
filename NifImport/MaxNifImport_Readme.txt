3ds Max Nif Importer v0.1

=========================
ABOUT
=========================
This is an importer for the NIF file format for 3ds Max 8.

It uses the NIF File Format Library and Tools library.

See the plugin INI file for advanced configuration.

=========================
REQUIREMENTS
=========================
3ds Max 8

=========================
INSTALL
=========================
Extract file to 3dsmax8 installation directory.

Its highly recommended that you edit the MaxNifTools.ini file to adjust paths 
and other settings to you machine.

Most importantly:
1. Remove irrelevant applications from the KnownApplications list

2. Fix the RootPaths, TexturePaths and TextureSearchPaths in the Applications for your machine.

   - The ini file has a substitution mechanism to make it easier to just change one location
     and refer to that path in other areas.
   
   - RootPaths is used to determine if the imported nif is in one of those directories
     if so it assumes the nif is from that app and uses those settings for import.
     
   - TexturePaths is used to test if the relative path in the nif is relative to that directory.
   
   - TextureSearchPaths will be recursively scanned for file names that match the extensions
     (this can be time consuming so keep it to only relevant directories to you).

=========================
UNINSTALL
=========================
Delete the installed files from 3dsmax directory. See the list below in the 
Installed Files section.

=========================
CHANGE LIST
=========================
v 0.1
 o Initial Revision

=========================
KNOWN ISSUES
=========================
o Bipeds do not import correctly.  Bones are doing better at the moment.

=========================
INSTALLED FILES
=========================
MaxNifImport_Readme.txt
plugins\MaxNifImport.dli
plugcfg\MaxNifTools.ini


=========================
WEBSITES
=========================
http://niftools.sourceforge.net/forum/index.php
http://niftools.sourceforge.net/forum/viewforum.php?f=6&sid=a7b7b59ba39233760ac22590ea8382e1


=========================
COPYRIGHT
=========================
Copyright (c) 2006, NIF File Format Library and Tools. All rights reserved.

=========================
LEGAL
=========================
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
