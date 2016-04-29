# * Copyright (c) 2016 Michael Wegner and Matthias Wolf
# *
# * Permission is hereby granted, free of charge, to any person obtaining a copy
# * of this software and associated documentation files (the "Software"), to deal
# * in the Software without restriction, including without limitation the rights
# * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# * copies of the Software, and to permit persons to whom the Software is
# * furnished to do so, subject to the following conditions:
# *
# * The above copyright notice and this permission notice shall be included in
# * all copies or substantial portions of the Software.
# *
# * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# * SOFTWARE.

import os
import fnmatch
import copy
import shutil

# Source files (excluding executables) will be gathered here
srcDir = "."
def getSourceFiles(target, optimize):
	source = []

	# walk source directory and find ONLY .cpp files
	for (dirpath, dirnames, filenames) in os.walk(srcDir):
	    for name in fnmatch.filter(filenames, "*.cpp"):
			source.append(os.path.join(dirpath, name))

	xpatterns = ["*Customization.cpp", "*Precalculation.cpp", "*OSMParser.cpp", "*Test.cpp"]
	excluded = []	

	for pattern in xpatterns:
		for name in fnmatch.filter(source, pattern):
			excluded.append(name)			

	source = [name for name in source if name not in excluded]

	# create build directory for build configuration
	buildDir = ".build{0}".format(optimize)
	VariantDir(buildDir, srcDir, duplicate=0)

	# modify source paths for build directory
	source = [name.replace(srcDir + "/", buildDir + "/") for name in source]	

	return source


AddOption("--compiler",
	dest="compiler",
	type="string",
	nargs=1,
	action="store",
	help="specify g++ version, standard is g++")

AddOption("--optimize",
          dest="optimize",
          type="string",
          nargs=1,
          action="store",
          help="specify the optimization level to build: D(ebug), O(ptimize), P(rofile)")

AddOption("--target",
          dest="target",
          type="string",
          nargs=1,
          action="store",
          help="select target to build")

env = Environment()
compiler = GetOption("compiler")

if compiler is None:
	compiler = "g++"

env["CC"] = compiler
env["CXX"] = compiler

########################################################################################
#										Libraries
########################################################################################

env.Append(LIBS = ["boost_iostreams"])
env.Append(LIBS = ["gomp"])

# specify correct path to your boost library
env.Append(CPPPATH = ["/usr/local/Cellar/boost/1.59.0/include"])
env.Append(LIBPATH = ["/usr/local/Cellar/boost/1.59.0/lib"])

if GetOption("clean"):
	if os.path.exists(".buildOpt"):
		shutil.rmtree('.buildOpt')
	if os.path.exists(".buildDbg"):
		shutil.rmtree('.buildDbg')
	if os.path.exists(".buildPro"):
		shutil.rmtree('.buildPro')

	if os.path.exists("customization/Customization.o"):		
		os.remove("customization/Customization.o")	
	if os.path.exists("precalculation/Precalculation.o"):		
		os.remove("precalculation/Precalculation.o")
	if os.path.exists("io/OSMParser.o"):		
		os.remove("io/OSMParser.o")
	if os.path.exists("test/DijkstraTest.o"):		
		os.remove("test/DijkstraTest.o")
	if os.path.exists("test/OverlayGraphTest.o"):		
		os.remove("test/OverlayGraphTest.o")
	if os.path.exists("test/QueryTest.o"):		
		os.remove("test/QueryTest.o")
	if os.path.exists("test/UnpackPathTest.o"):		
		os.remove("test/UnpackPathTest.o")
	exit()

try:
    optimize = GetOption("optimize")
except:
    print("ERROR: Missing option --optimize=<LEVEL>. Available levels are: Opt, Dbg and Pro.")
    exit()

try:
    target = GetOption("target")
except:
    print("ERROR: Missing option --target=<TARGET>. Available targets are: CRP and Tests.")
    exit()

commonCFlags = ["-c", "-fmessage-length=0", "-std=c++11", "-fPIC", "-fopenmp"]
commonCppFlags = ["-std=c++11", "-Wall", "-c", "-fmessage-length=0", "-fPIC", "-fopenmp"]

debugCppFlags = ["-O0", "-g3"]
debugCFlags = ["-O0", "-g3"]

optimizedCppFlags = ["-O3", "-DNDEBUG"]
optimizedCFlags = ["-O3"]

profileCppFlags = ["-O2", "-DNDEBUG", "-g", "-pg"]
profileCFlags = ["-O2", "-DNDEBUG", "-g", "-pg"]

env.Append(CFLAGS = commonCFlags)
env.Append(CPPFLAGS = commonCppFlags)

if optimize == "Dbg":
    env.Append(CFLAGS = debugCFlags)
    env.Append(CPPFLAGS = debugCppFlags)
elif optimize == "Opt":
    env.Append(CFLAGS = optimizedCFlags)
    env.Append(CPPFLAGS = optimizedCppFlags)
elif optimize == "Pro":
	 env.Append(CFLAGS = profileCFlags)
	 env.Append(CPPFLAGS = profileCppFlags)
else:
    print("ERROR: invalid optimize: %s" % optimize)
    exit()


source = getSourceFiles(target, optimize)

if target == "CRP":
	targetSource = list(source)
	targetSource.append(os.path.join(srcDir, "customization/Customization.cpp"))
	env.Program("deploy/customization", targetSource)


	targetSource = list(source)
	targetSource.append(os.path.join(srcDir, "precalculation/Precalculation.cpp"))	
	env.Program("deploy/precalculation", targetSource)

	targetSource = list(source)
	targetSource.append(os.path.join(srcDir, "io/OSMParser.cpp"))
	env.Program("deploy/osmparser", targetSource)

elif target == "QueryTest":
	env.Append(CPPFLAGS = ["-DQUERYTEST"])
	source.append(os.path.join(srcDir, "test/QueryTest.cpp"))
	env.Program("deploy/querytest", source)

elif target == "DijkstraTest":
	env.Append(CPPFLAGS = ["-DQUERYTEST"])
	source.append(os.path.join(srcDir, "test/DijkstraTest.cpp"))
	env.Program("deploy/dijkstratest", source)	

elif target == "UnpackPathTest":
	env.Append(CPPFLAGS = ["-DUNPACKPATHTEST"])
	source.append(os.path.join(srcDir, "test/UnpackPathTest.cpp"))
	env.Program("deploy/unpackpathtest", source)

elif target == "OverlayGraphTest":
	env.Append(CPPFLAGS = ["-DQUERYTEST"])
	source.append(os.path.join(srcDir, "test/OverlayGraphTest.cpp"))
	env.Program("deploy/overlaygraphtest", source)
else:
	print("ERROR: unknown target: {0}".format(target))
	exit(1)
