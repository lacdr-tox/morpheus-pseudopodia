Morpheus
========

Morpheus is a modeling and simulation environment for the study of multiscale and multicellular systems.
For further information look at https://morpheus.gitlab.io .

Morpheus has been developed by Jörn Starruß and Walter de Back at the Center for High Performance Computing at the Technische Universität Dresden, Germany.


<img src="https://gitlab.com/morpheus.lab/morpheus/uploads/3a7c4af0d6f9507f6143a67720d629e1/Morpheus.png" width="60%"/>
*Morpheus graphical user interface. Model editor (top) and results browser (bottom)*

Looking for latest stable release?
=====================

If you are looking for the latest stable release of Morpheus, and do not necessarily need the source code, please download the precompiled packages for MS Windows, Mac OSX and Linux are available on the [download page](https://morpheus.gitlab.io/#download). 


Resources
=========

Morpheus is actively supported and provides help for users and developers:

- [User forum](https://groups.google.com/forum/#!forum/morpheus-users): Questions and answers on modeling with Morpheus
- [Issue tracker](https://gitlab.com/morpheus.lab/morpheus/issues): Bug reports and feature requests

Documentation for users as well as plugin developers here found on the [gitlab wiki](https://gitlab.com/morpheus.lab/morpheus/wikis/home) and on the on the [old wiki](https://imc.zih.tu-dresden.de/wiki/morpheus):

- [User manual](https://gitlab.com/morpheus.lab/morpheus/wikis/user-manual) (under construction)
- [Plugin dev guide](https://gitlab.com/morpheus.lab/morpheus/wikis/dev-guide) (under construction)

These docs may not reflect the latest state and are currently being updated.

To find out more about Morpheus, please take a look at the home page:

- [Homepage](https://morpheus.gitlab.io): Binary downloads and news.


Install
=======

build tools required:
  - g++ (>= 4.6)
  - cmake (>= 2.8)
  - cmake-curses-gui (for ccmake, optional)
  - xsltproc
  - xmllint (optional)
  - doxygen
  - git

Libraries required (debian package notation):
  - zlib1g-dev libtiff-dev graphviz-dev
  - libqt4-dev libqt4-sql-sqlite libqt4-network libqt4-webkit libqt4-svg libqt4-xml libqt4-dev-bin qt4-dev-tools
  
```
  git clone https://gitlab.com/morpheus.lab/morpheus.git morpheus
  cd morpheus
  mkdir build
  cd build
  cmake ..
  make && sudo make install
```

Building on Debian based systems
--------------------------------
To install all dependencies on Ubuntu 16.04 run:
```  
sudo apt-get install g++ cmake cmake-curses-gui xsltproc libxml2-utils doxygen git zlib1g-dev libtiff5-dev libgraphviz-dev libqt4-dev libqt4-sql-sqlite libqt4-network libqtwebkit-dev libqt4-svg libqt4-xml libqt4-dev-bin qt4-dev-tools libsbml5-dev
``` 
To install all dependencies on Ubuntu 14.04 and other Debian based systems run:
```  
sudo apt-get install g++ cmake cmake-curses-gui xsltproc libxml2-utils doxygen git zlib1g-dev libtiff-dev libgraphviz-dev libqt4-dev libqt4-sql-sqlite libqt4-network libqt4-webkit libqt4-svg libqt4-xml libqt4-dev-bin qt4-dev-tools
``` 

Building/Installing on Arch/Manjaro
------------------------------------

Morpheus has been added to AUR (https://aur.archlinux.org/packages/morpheus-modeling/) so to install the dependencies and build from source simply run

```
yaourt morpheus-modeling
```

How to cite Morpheus
---------------------------

Please use this reference when citing Morpheus:

> J. Starruß, W. de Back, L. Brusch and A. Deutsch.  
> Morpheus: a user-friendly modeling environment for multiscale and multicellular systems biology.  
> Bioinformatics, 30(9):1331-1332, 2014. https://doi.org/10.1093/bioinformatics/btt772

Additionaly, use the Morpheus [Research Resource Identifier (RRID)](https://scicrunch.org/resources) ([SCR_014975](https://scicrunch.org/browse/resources/SCR_014975)) in your method section.
Include the version number or commit hash for reproducability. Valid examples are:

> Morpheus, RRID:SCR_014975  
> Morpheus, v1.9.2, RRID:SCR_014975  
> Morpheus, e45739bc, RRID:SCR_014975


<!--  StatCounter -->
<script type="text/javascript">
var sc_project=10858269; 
var sc_invisible=1; 
var sc_security="392b0df5"; 
var scJsHost = (("https:" == document.location.protocol) ?
"https://secure." : "http://www.");
document.write("<sc"+"ript type='text/javascript' src='" +
scJsHost+
"statcounter.com/counter/counter.js'></"+"script>");
</script>
<noscript><div class="statcounter"><a title=""
href="http://statcounter.com/shopify/" target="_blank"><img
class="statcounter"
src="http://c.statcounter.com/10858269/0/392b0df5/1/"
alt=""></a></div></noscript>
<!-- End of StatCounter Code -->
