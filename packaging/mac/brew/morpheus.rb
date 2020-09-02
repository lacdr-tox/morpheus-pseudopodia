class Morpheus < Formula
  desc "Modelling environment for Multi-Cellular Systems Biology"
  homepage "https://morpheus.gitlab.io"
  url "https://gitlab.com/morpheus.lab/morpheus/-/archive/v2.2.0-beta2/morpheus-v2.2.0-beta2.tar.gz"
  sha256 "b99b8cba61d206c084882f066d04914cee03eab706e55f3c161445f8d929d981"
  version "2.1.92"

  head "https://gitlab.com/morpheus.lab/morpheus.git", :branch => "develop"

  depends_on "boost" => :build
  depends_on "cmake" => :build
  depends_on "doxygen" => :build
  depends_on "gnuplot" # Runtime dependencies
#   depends_on "graphviz"
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
