/*
 * Copyright (C) 2008, Willow Garage, Inc.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the names of Stanford University or Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "rospack/rospack.h"
#include "utils.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

namespace po = boost::program_options;

namespace rospack
{

bool parse_args(int argc, char** argv, 
                rospack::Rosstackage& rp,
                po::variables_map& vm);

bool
rospack_run(int argc, char** argv, rospack::Rosstackage& rp, std::string& output)
{
  po::variables_map vm;

  if(!parse_args(argc, argv, rp, vm))
    return false;

  bool quiet = (vm.count("quiet")==1);
  rp.setQuiet(quiet);

  std::string command;
  std::string package;
  bool package_given = false;
  bool deps_only = false;
  std::string lang;
  std::string attrib;
  std::string top;
  std::string target;
  bool zombie_only = false;
  std::string length_str;
  int length;
  if(vm.count("command"))
    command = vm["command"].as<std::string>();
  if(!command.size())
  {
    rp.log_error( "no command given.  Try 'rospack help'");
    return true;
  }
  // For some commands, we force a crawl.  Definitely anything that does a
  // depends-on calculation.
  bool force = false;
  if((command == "profile") ||
     (command == "depends-on") ||
     (command == "depends-on1") ||
     (command == "langs") ||
     (command == "list-duplicates"))
    force = true;

  if(vm.count("package"))
  {
    package = vm["package"].as<std::string>();
    package_given = true;
  }
  else
  {
    // try to determine package from directory context
    rp.inStackage(package);
  }
  if(vm.count("deps-only"))
    deps_only = true;
  if(vm.count("lang"))
    lang = vm["lang"].as<std::string>();
  if(vm.count("attrib"))
    attrib = vm["attrib"].as<std::string>();
  if(vm.count("top"))
    top = vm["top"].as<std::string>();
  if(vm.count("target"))
    target = vm["target"].as<std::string>();
  if(vm.count("zombie-only"))
    zombie_only = true;
  if(vm.count("length"))
  {
    length_str = vm["length"].as<std::string>();
    length = atoi(length_str.c_str());
  }
  else
  {
    if(zombie_only)
      length = -1;
    else
      length = 20;
  }
  
  // COMMAND: help
  if(command == "help")
  {
    if(package_given || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    output.append(rp.usage());
    return true;
  }

  std::vector<std::string> search_path;
  if(!rp.getSearchPathFromEnv(search_path))
    return false;

  // COMMAND: profile
  if(command == "profile")
  {
    if(package_given || target.size() || top.size() || 
       deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> dirs;
    if(rp.profile(search_path, zombie_only, length, dirs))
      return false;
    for(std::vector<std::string>::const_iterator it = dirs.begin();
        it != dirs.end();
        ++it)
      output.append((*it) + "\n");
    return true;
  }

  // We crawl here because profile (above) does its own special crawl.
  rp.crawl(search_path, force);
  
  // COMMAND: find [package]
  if(command == "find")
  {
    if(!package.size())
    {
      rp.log_error( "no package/stack given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::string path;
    if(!rp.find(package, path))
      return false;
    output.append(path + "\n");
    return true;
  }
  // COMMAND: list
  else if(command == "list")
  {
    if(package_given || target.size() || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::pair<std::string, std::string> > list;
    rp.list(list);
    for(std::vector<std::pair<std::string, std::string> >::const_iterator it = list.begin();
        it != list.end();
        ++it)
    {
      output.append(it->first + " " + it->second + "\n");
    }
    return true;
  }
  // COMMAND: list-names
  else if(command == "list-names")
  {
    if(package_given || target.size() || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::pair<std::string, std::string> > list;
    rp.list(list);
    for(std::vector<std::pair<std::string, std::string> >::const_iterator it = list.begin();
        it != list.end();
        ++it)
    {
      output.append(it->first + "\n");
    }
    return true;
  }
  // COMMAND: list-duplicates
  else if(command == "list-duplicates")
  {
    if(package_given || target.size() || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> dups;
    rp.listDuplicates(dups);
    // if there are dups, list-duplicates prints them and returns non-zero
    for(std::vector<std::string>::const_iterator it = dups.begin();
        it != dups.end();
        ++it)
    {
      output.append(*it + "\n");
    }
    return true;
  }
  // COMMAND: langs
  else if(rp.getName() == ROSPACK_NAME && command == "langs")
  {
    if(package_given || target.size() || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> deps;
    if(!rp.depsOn("roslang", true, deps))
      return false;
    const char* ros_lang_disable;
    if((ros_lang_disable = getenv("ROS_LANG_DISABLE")))
    {
      std::vector<std::string> disable_langs;
      boost::split(disable_langs, ros_lang_disable,
                   boost::is_any_of(":"),
                   boost::token_compress_on);
      std::vector<std::string>::iterator it = deps.begin();
      while(it != deps.end())
      {
        if(std::find(disable_langs.begin(), disable_langs.end(), *it) != 
           disable_langs.end())
          it = deps.erase(it);
        else
          ++it;
      }
    }
    for(std::vector<std::string>::const_iterator it = deps.begin();
        it != deps.end();
        ++it)
      output.append(*it + " ");
    output.append("\n");
    return true;
  }
  // COMMAND: depends [package] (alias: deps)
  else if(command == "depends" || command == "deps" || 
          command == "depends1" || command == "deps1")
  {
    if(!package.size())
    {
      rp.log_error( "no package/stack given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> deps;
    if(!rp.deps(package, (command == "depends1" || command == "deps1"), deps))
      return false;
    for(std::vector<std::string>::const_iterator it = deps.begin();
        it != deps.end();
        ++it)
      output.append(*it + "\n");
    return true;
  }
  // COMMAND: depends-manifests [package] (alias: deps-manifests)
  else if(command == "depends-manifests" || command == "deps-manifests")
  {
    if(!package.size())
    {
      rp.log_error( "no package/stack given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> manifests;
    if(!rp.depsManifests(package, false, manifests))
      return false;
    for(std::vector<std::string>::const_iterator it = manifests.begin();
        it != manifests.end();
        ++it)
        output.append(*it + " ");
    output.append("\n");
    return true;
  }
  // COMMAND: depends-msgsrv [package] (alias: deps-msgsrv)
  else if(rp.getName() == ROSPACK_NAME && 
          (command == "depends-msgsrv" || command == "deps-msgsrv"))
  {
    if(!package.size())
    {
      rp.log_error( "no package given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> gens;
    if(!rp.depsMsgSrv(package, false, gens))
      return false;
    for(std::vector<std::string>::const_iterator it = gens.begin();
        it != gens.end();
        ++it)
      output.append(*it + " ");
    output.append("\n");
    return true;
  }
  // COMMAND: depends-indent [package] (alias: deps-indent)
  else if(command == "depends-indent" || command == "deps-indent")
  {
    if(!package.size())
    {
      rp.log_error( "no package/stack given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> deps;
    if(!rp.depsIndent(package, false, deps))
      return false;
    for(std::vector<std::string>::const_iterator it = deps.begin();
        it != deps.end();
        ++it)
      output.append(*it + "\n");
    return true;
  }
  // COMMAND: depends-why [package] (alias: deps-why)
  else if(command == "depends-why" || command == "deps-why")
  {
    if(!package.size() || !target.size())
    {
      rp.log_error( "no package/stack or target given");
      return false;
    }
    if(top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::string why_output;
    if(!rp.depsWhy(package, target, why_output))
      return false;
    output.append(why_output);
    return true;
  }
  // COMMAND: rosdep [package] (alias: rosdeps)
  // COMMAND: rosdep0 [package] (alias: rosdeps0)
  else if(rp.getName() == ROSPACK_NAME && 
          (command == "rosdep" || command == "rosdeps" ||
           command == "rosdep0" || command == "rosdeps0"))
  {
    if(!package.size())
    {
      rp.log_error( "no package given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> rosdeps;
    if(!rp.rosdeps(package, (command == "rosdep0" || command == "rosdeps0"), rosdeps))
      return false;
    for(std::vector<std::string>::const_iterator it = rosdeps.begin();
        it != rosdeps.end();
        ++it)
      output.append(*it + "\n");
    return true;
  }
  // COMMAND: vcs [package]
  // COMMAND: vcs0 [package]
  else if(rp.getName() == ROSPACK_NAME && 
          (command == "vcs" || command == "vcs0"))
  {
    if(!package.size())
    {
      rp.log_error( "no package given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> vcs;
    if(!rp.vcs(package, (command == "vcs0"), vcs))
      return false;
    for(std::vector<std::string>::const_iterator it = vcs.begin();
        it != vcs.end();
        ++it)
      output.append(*it + "\n");
    return true;
  }
  // COMMAND: depends-on [package]
  // COMMAND: depends-on1 [package]
  else if(command == "depends-on" || command == "depends-on1")
  {
    if(!package.size())
    {
      rp.log_error( "no package/stack given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> deps;
    if(!rp.depsOn(package, (command == "depends-on1"), deps))
      return false;
    for(std::vector<std::string>::const_iterator it = deps.begin();
        it != deps.end();
        ++it)
      output.append(*it + "\n");
    return true;
  }
  // COMMAND: export [--deps-only] --lang=<lang> --attrib=<attrib> [package]
  else if(rp.getName() == ROSPACK_NAME && command == "export")
  {
    if(!package.size() || !lang.size() || !attrib.size())
    {
      rp.log_error( "no package / lang / attrib given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || zombie_only)
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> flags;
    if(!rp.exports(package, lang, attrib, deps_only, flags))
      return false;
    for(std::vector<std::string>::const_iterator it = flags.begin();
        it != flags.end();
        ++it)
      output.append(*it + " ");
    output.append("\n");
    return true;
  }
  // COMMAND: plugins --attrib=<attrib> [--top=<toppkg>] [package]
  else if(rp.getName() == ROSPACK_NAME && command == "plugins")
  {
    if(!package.size() || !attrib.size())
    {
      rp.log_error( "no package / attrib given");
      return false;
    }
    if(target.size() || length_str.size() || zombie_only)
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> flags;
    if(!rp.plugins(package, attrib, top, flags))
      return false;
    for(std::vector<std::string>::const_iterator it = flags.begin();
        it != flags.end();
        ++it)
      output.append(*it + "\n");
    return true;
  }
  // COMMAND: cflags-only-I [--deps-only] [package]
  else if(rp.getName() == ROSPACK_NAME && command == "cflags-only-I")
  {
    if(!package.size())
    {
      rp.log_error( "no package given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || zombie_only)
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> flags;
    if(!rp.exports(package, "cpp", "cflags", deps_only, flags))
      return false;
    std::string combined;
    for(std::vector<std::string>::const_iterator it = flags.begin();
        it != flags.end();
        ++it)
      combined.append(*it + " ");
    std::string result;
    parse_compiler_flags(combined, "-I", true, false, result);
    output.append(result + "\n");
    return true;
  }
  // COMMAND: cflags-only-other [--deps-only] [package]
  else if(rp.getName() == ROSPACK_NAME && command == "cflags-only-other")
  {
    if(!package.size())
    {
      rp.log_error( "no package given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || zombie_only)
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> flags;
    if(!rp.exports(package, "cpp", "cflags", deps_only, flags))
      return false;
    std::string combined;
    for(std::vector<std::string>::const_iterator it = flags.begin();
        it != flags.end();
        ++it)
      combined.append(*it + " ");
    std::string result;
    parse_compiler_flags(combined, "-I", false, false, result);
    output.append(result + "\n");
    return true;
  }
  // COMMAND: libs-only-L [--deps-only] [package]
  else if(rp.getName() == ROSPACK_NAME && command == "libs-only-L")
  {
    if(!package.size())
    {
      rp.log_error( "no package given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || zombie_only)
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> flags;
    if(!rp.exports(package, "cpp", "lflags", deps_only, flags))
      return false;
    std::string combined;
    for(std::vector<std::string>::const_iterator it = flags.begin();
        it != flags.end();
        ++it)
      combined.append(*it + " ");
    std::string result;
    parse_compiler_flags(combined, "-L", true, false, result);
    output.append(result + "\n");
    return true;
  }
  // COMMAND: libs-only-l [--deps-only] [package]
  else if(rp.getName() == ROSPACK_NAME && command == "libs-only-l")
  {
    if(!package.size())
    {
      rp.log_error( "no package given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || zombie_only)
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> flags;
    if(!rp.exports(package, "cpp", "lflags", deps_only, flags))
      return false;
    std::string combined;
    for(std::vector<std::string>::const_iterator it = flags.begin();
        it != flags.end();
        ++it)
      combined.append(*it + " ");
    std::string result;
    parse_compiler_flags(combined, "-l", true, true, result);
    output.append(result + "\n");
    return true;
  }
  // COMMAND: libs-only-other [--deps-only] [package]
  else if(rp.getName() == ROSPACK_NAME && command == "libs-only-other")
  {
    if(!package.size())
    {
      rp.log_error( "no package given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || zombie_only)
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::vector<std::string> flags;
    if(!rp.exports(package, "cpp", "lflags", deps_only, flags))
      return false;
    std::string combined;
    for(std::vector<std::string>::const_iterator it = flags.begin();
        it != flags.end();
        ++it)
      combined.append(*it + " ");
    std::string intermediate;
    parse_compiler_flags(combined, "-L", false, false, intermediate);
    std::string result;
    parse_compiler_flags(intermediate, "-l", false, false, result);
    output.append(result + "\n");
    return true;
  }
  // COMMAND: contents [stack]
  else if(rp.getName() == ROSSTACK_NAME && command == "contents")
  {
    if(!package.size())
    {
      rp.log_error( "no stack given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }

    std::vector<std::string> packages;
    rp.contents(package, packages);
    for(std::vector<std::string>::const_iterator it = packages.begin();
        it != packages.end();
        ++it)
      output.append(*it + "\n");
    return true;
  }
  // COMMAND: contains [package]
  else if(rp.getName() == ROSSTACK_NAME && 
          ((command == "contains") || (command == "contains-path")))
  {
    if(!package.size())
    {
      rp.log_error( "no package given");
      return false;
    }
    if(target.size() || top.size() || length_str.size() || 
       zombie_only || deps_only || lang.size() || attrib.size())
    {
      rp.log_error( "invalid option(s) given");
      return false;
    }
    std::string name, path;
    if(!rp.contains(package, name, path))
      return false;
    if(command == "contains")
      output.append(name + "\n");
    else // command == "contains-path"
      output.append(path + "\n");
    return true;
  }
  else
  {
    rp.log_error(std::string("command ") + command + " not implemented");
    return false;
  }
}

bool
parse_args(int argc, char** argv, 
           rospack::Rosstackage& rp, po::variables_map& vm)
{
  po::options_description desc("Allowed options");
  desc.add_options()
          ("command", po::value<std::string>(), "command")
          ("package", po::value<std::string>(), "package")
          ("target", po::value<std::string>(), "target")
          ("deps-only", "deps-only")
          ("lang", po::value<std::string>(), "lang")
          ("attrib", po::value<std::string>(), "attrib")
          ("top", po::value<std::string>(), "top")
          ("length", po::value<std::string>(), "length")
          ("zombie-only", "zombie-only")
          ("quiet,q", "quiet");

  po::positional_options_description pd;
  pd.add("command", 1).add("package", 1);
  try
  {
    po::store(po::command_line_parser(argc, argv).options(desc).positional(pd).run(), vm);
  }
  catch(boost::program_options::error e)
  {
    rp.log_error( std::string("failed to parse command-line options: ") + e.what());
    return false;
  }
  po::notify(vm);

  return true;
}

const char* 
Rospack::usage()
{
  return "USAGE: rospack <command> [options] [package]\n"
          "  Allowed commands:\n"
          "    help\n"
          "    find [package]\n"
          "    list\n"
          "    list-names\n"
          "    list-duplicates\n"
          "    langs\n"
          "    depends [package] (alias: deps)\n"
          "    depends-manifests [package] (alias: deps-manifests)\n"
          "    depends-msgsrv [package] (alias: deps-msgsrv)\n"
          "    depends1 [package] (alias: deps1)\n"
          "    depends-indent [package] (alias: deps-indent)\n"
          "    depends-why --target=<target> [package] (alias: deps-why)\n"
          "    rosdep [package] (alias: rosdeps)\n"
          "    rosdep0 [package] (alias: rosdeps0)\n"
          "    vcs [package]\n"
          "    vcs0 [package]\n"
          "    depends-on [package]\n"
          "    depends-on1 [package]\n"
          "    export [--deps-only] --lang=<lang> --attrib=<attrib> [package]\n"
          "    plugins --attrib=<attrib> [--top=<toppkg>] [package]\n"
          "    cflags-only-I [--deps-only] [package]\n"
          "    cflags-only-other [--deps-only] [package]\n"
          "    libs-only-L [--deps-only] [package]\n"
          "    libs-only-l [--deps-only] [package]\n"
          "    libs-only-other [--deps-only] [package]\n"
          "    profile [--length=<length>] [--zombie-only]\n"
          "  Extra options:\n"
          "    -q     Quiets error reports.\n\n"
          " If [package] is omitted, the current working directory\n"
          " is used (if it contains a manifest.xml).\n\n";
}

const char* 
Rosstack::usage()
{
  return "USAGE: rosstack [options] <command> [stack]\n"
          "  Allowed commands:\n"
          "    help\n"
          "    find [stack]\n"
          "    contents [stack]\n"
          "    list\n"
          "    list-names\n"
          "    depends [stack] (alias: deps)\n"
          "    depends-manifests [stack] (alias: deps-manifests)\n"
          "    depends1 [stack] (alias: deps1)\n"
          "    depends-indent [stack] (alias: deps-indent)\n"
          "    depends-why --target=<target> [stack] (alias: deps-why)\n"
          "    depends-on [stack]\n"
          "    depends-on1 [stack]\n"
          "    contains [package]\n"
          "    contains-path [package]\n"
          "    profile [--length=<length>] \n\n"
          " If [stack] is omitted, the current working directory\n"
          " is used (if it contains a stack.xml).\n\n";
}

}