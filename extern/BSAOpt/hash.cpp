/* Version: MPL 1.1/LGPL 3.0
 *
 * "The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is BSAopt.
 *
 * The Initial Developer of the Original Code is
 * Ethatron <niels@paradice-insight.us>. Portions created by The Initial
 * Developer are Copyright (C) 2011 The Initial Developer.
 * All Rights Reserved.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Library General Public License Version 3 license (the
 * "LGPL License"), in which case the provisions of LGPL License are
 * applicable instead of those above. If you wish to allow use of your
 * version of this file only under the terms of the LGPL License and not
 * to allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and replace
 * them with the notice and other provisions required by the LGPL License.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under either the MPL or the LGPL License."
 */

#include <cstdint>
#include <algorithm>

#include "hash.hpp"

std::uint32_t GenOBHashStr(const std::string& s) {
    std::uint32_t hash = 0;

  for (std::size_t i = 0; i < s.length(); i++) {
    hash *= 0x1003F;
    hash += (unsigned char)s[i];
  }

  return hash;
}

std::uint64_t GenOBHashPair(const std::string& fle, const std::string& ext) {
    std::uint64_t hash = 0;

  if (fle.length() > 0) {
    hash = (std::uint64_t)(
      (((unsigned char)fle[fle.length() - 1]) * 0x1) +
      ((fle.length() > 2 ? (unsigned char)fle[fle.length() - 2] : (unsigned char)0) * 0x100) +
      (fle.length() * 0x10000) +
      (((unsigned char)fle[0]) * 0x1000000)
    );

    if (fle.length() > 3) {
      hash += (std::uint64_t)(GenOBHashStr(fle.substr(1, fle.length() - 3)) * 0x100000000);
    }
  }

  if (ext.length() > 0) {
    hash += (std::uint64_t)(GenOBHashStr(ext) * 0x100000000LL);

    unsigned char i = 0;
    if (ext == ".nif") i = 1;
    if (ext == ".kf" ) i = 2;
    if (ext == ".dds") i = 3;
    if (ext == ".wav") i = 4;

    if (i != 0) {
      unsigned char a = (unsigned char)(((i & 0xfc ) << 5) + (unsigned char)((hash & 0xff000000) >> 24));
      unsigned char b = (unsigned char)(((i & 0xfe ) << 6) + (unsigned char)( hash & 0x000000ff)       );
      unsigned char c = (unsigned char)(( i          << 7) + (unsigned char)((hash & 0x0000ff00) >>  8));

      hash -= hash & 0xFF00FFFF;
      hash += (std::uint32_t)((a << 24) + b + (c << 8));
    }
  }

  return hash;
}

std::uint64_t GenOBHash(const std::string& path, std::string& file) {
  std::transform(file.begin(), file.end(), file.begin(), ::tolower);
  std::replace(file.begin(), file.end(), '/', '\\');

  std::string fle;
  std::string ext;

  const char *_fle = file.data();
  const char *_ext = strrchr(_fle, '.');
  if (_ext) {
    ext = file.substr((0 + _ext) - _fle);
    fle = file.substr(0, ( _ext) - _fle);
  }
  else {
    ext = "";
    fle = file;
  }

  if (path.length() && fle.length())
    return GenOBHashPair(path + "\\" + fle, ext);
  else
    return GenOBHashPair(path +        fle, ext);
}
