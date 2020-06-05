class Morpheus < Formula
  desc "Modelling environment for Multi-Cellular Systems Biology"
  homepage "https://morpheus.gitlab.io"
  url "https://gitlab.com/morpheus.lab/morpheus/-/archive/v2.1.91/morpheus-v2.1.91.tar.gz"
  sha256 "4bedf09eb13fa606a02753cb32c0c3a1f77c3a3005b8799ee537c1135f5ba799"
  version "2.1.91"

  head "https://gitlab.com/morpheus.lab/morpheus.git", :branch => "develop"

  depends_on "boost" => :build
  depends_on "cmake" => :build
  depends_on "doxygen" => :build
  depends_on "gnuplot" # Runtime dependencies
  depends_on "graphviz"
  depends_on "libtiff"
  depends_on "qt"
  depends_on "libomp" => :recommended

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
