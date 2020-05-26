class Morpheus < Formula
  desc "Modelling environment for Multi-Cellular Systems Biology"
  homepage "https://morpheus.gitlab.io"
  url "https://gitlab.com/morpheus.lab/morpheus/-/archive/v2.2.0-b2/morpheus-v2.2.0-b2.tar.gz"
  sha256 "3807a072368eb597d7960ad84617f37ee04b195e2807fa2539f608d71a01dd7d"
  version "2.2.0-b2"

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
