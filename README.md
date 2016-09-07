Morpheus
========

Morpheus is a modeling and simulation environment for the study of multiscale and multicellular systems.
For further information look at https://imc.zih.tu-dresden.de/wiki/morpheus .

Morpheus has been developed by Jörn Starruß and Walter de Back at the Center for High Performance Computing at the Technische Universität Dresden, Germany.


<img src="https://gitlab.com/morpheus.lab/morpheus/uploads/3a7c4af0d6f9507f6143a67720d629e1/Morpheus.png" width="60%"/>
*Morpheus graphical user interface. Model editor (top) and results browser (bottom)*

Install
=======

build tools required:
  - g++ (>= 4.6)
  - cmake (>= 2.6)
  - cmake-curses-gui (for ccmake, optional)
  - xsltproc
  - xmlint (optional)
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
To install all dependencies on Ubuntu 14.04 and other Debian based systems run:
```  
sudo apt-get install g++ cmake cmake-curses-gui xsltproc xmlint doxygen git zlib1g-dev libtiff-dev libgraphviz-dev libqt4-dev libqt4-sql-sqlite libqt4-network libqt4-webkit libqt4-svg libqt4-xml libqt4-dev-bin qt4-dev-tool
``` 

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
<noscript><div class="statcounter"><a title="shopify
analytics ecommerce tracking"
href="http://statcounter.com/shopify/" target="_blank"><img
class="statcounter"
src="http://c.statcounter.com/10858269/0/392b0df5/1/"
alt="shopify analytics ecommerce
tracking"></a></div></noscript>
<!-- End of StatCounter Code -->
