class Morpheus < Formula
  desc "Modelling environment for Multi-Cellular Systems Biology"
  homepage "https://morpheus.gitlab.io"
  url "https://gitlab.com/morpheus.lab/morpheus/-/archive/v2.2.0-beta3/morpheus-v2.2.0-beta3.tar.gz"
  sha256 "039ecaecdce0baeb567fe271fa4545b8b5131f78eeadf281f1752630582b4275"
  version "2.1.93"
  license "BSD-3-clause"
  head "https://gitlab.com/morpheus.lab/morpheus.git", :branch => "develop"

  depends_on "boost" => :build
  depends_on "cmake" => :build
  depends_on "doxygen" => :build
  depends_on "gnuplot" # Runtime dependencies
#   depends_on "graphviz"
  depends_on "libtiff"
  depends_on "qt"
  depends_on "libomp" => :recommended
  depends_on "ffmpeg" => :recommended

  uses_from_macos "bzip2"
  uses_from_macos "libxml2"
  uses_from_macos "zlib"

  def install
    system "cmake", ".", *std_cmake_args
    system "make", "install"
  end

  #test do
  #  system "morpheus/morpheus", "--version"
  #end
end
