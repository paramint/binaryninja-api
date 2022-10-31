# Copyright (c) 2015-2022 Vector 35 Inc
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

import ctypes
import traceback

# Binary Ninja components
import binaryninja

from . import _binaryninjacore as core
from . import binaryview
from . import types
from .log import log_error
from .architecture import Architecture, CoreArchitecture
from .platform import Platform
from typing import Union, Optional, Tuple


def get_qualified_name(names):
	"""
	``get_qualified_name`` gets a qualified name for the provided name list.

	:param names: name list to qualify
	:type names: list(str)
	:return: a qualified name
	:rtype: str
	:Example:

		>>> type, name = demangle_ms(Architecture["x86_64"], "?testf@Foobar@@SA?AW4foo@1@W421@@Z")
		>>> get_qualified_name(name)
		'Foobar::testf'
		>>>
	"""
	return "::".join(names)


def demangle_ms(archOrPlatform:Union[Architecture, Platform], mangled_name:str, options=False):
	"""
	``demangle_ms`` demangles a mangled Microsoft Visual Studio C++ name to a Type object.

	:param Union[Architecture, Platform] archOrPlatform: Architecture or Platform for the symbol. Required for pointer/integer sizes and calling conventions.
	:param str mangled_name: a mangled Microsoft Visual Studio C++ name
	:param options: (optional) Whether to simplify demangled names : None falls back to user settings, a BinaryView uses that BinaryView's settings, or a boolean to set it directly
	:type options: Tuple[bool, BinaryView, None]
	:return: returns tuple of (Type, demangled_name) or (None, mangled_name) on error
	:rtype: Tuple
	:Example:

		>>> demangle_ms(Platform["x86_64"], "?testf@Foobar@@SA?AW4foo@1@W421@@Z")
		(<type: public: static enum Foobar::foo __cdecl (enum Foobar::foo)>, ['Foobar', 'testf'])
		>>>
	"""
	handle = ctypes.POINTER(core.BNType)()
	outName = ctypes.POINTER(ctypes.c_char_p)()
	outSize = ctypes.c_ulonglong()
	names = []

	demangle = core.BNDemangleMS
	demangleWithOptions = core.BNDemangleMSWithOptions

	if isinstance(archOrPlatform, Platform):
		demangle = core.BNDemangleMSPlatform

	if (
	    isinstance(options, binaryview.BinaryView) and demangleWithOptions(
	        archOrPlatform.handle, mangled_name, ctypes.byref(handle), ctypes.byref(outName), ctypes.byref(outSize), options
	    )
	) or (
	    isinstance(options, bool) and demangle(
	        archOrPlatform.handle, mangled_name, ctypes.byref(handle), ctypes.byref(outName), ctypes.byref(outSize), options
	    )
	) or (
	    options is None and demangleWithOptions(
	        archOrPlatform.handle, mangled_name, ctypes.byref(handle), ctypes.byref(outName), ctypes.byref(outSize), None
	    )
	):
		for i in range(outSize.value):
			names.append(outName[i].decode('utf8'))  # type: ignore
		core.BNFreeDemangledName(ctypes.byref(outName), outSize.value)
		return (types.Type.create(core.BNNewTypeReference(handle)), names)
	return (None, mangled_name)


def demangle_gnu3(arch, mangled_name, options=None):
	"""
	``demangle_gnu3`` demangles a mangled name to a Type object.

	:param Architecture arch: Architecture for the symbol. Required for pointer and integer sizes.
	:param str mangled_name: a mangled GNU3 name
	:param options: (optional) Whether to simplify demangled names : None falls back to user settings, a BinaryView uses that BinaryView's settings, or a boolean to set it directly
	:type options: Tuple[bool, BinaryView, None]
	:return: returns tuple of (Type, demangled_name) or (None, mangled_name) on error
	:rtype: Tuple
	"""
	handle = ctypes.POINTER(core.BNType)()
	outName = ctypes.POINTER(ctypes.c_char_p)()
	outSize = ctypes.c_ulonglong()
	names = []
	if (
	    isinstance(options, binaryview.BinaryView) and core.BNDemangleGNU3WithOptions(
	        arch.handle, mangled_name, ctypes.byref(handle), ctypes.byref(outName), ctypes.byref(outSize), options
	    )
	) or (
	    isinstance(options, bool) and core.BNDemangleGNU3(
	        arch.handle, mangled_name, ctypes.byref(handle), ctypes.byref(outName), ctypes.byref(outSize), options
	    )
	) or (
	    options is None and core.BNDemangleGNU3WithOptions(
	        arch.handle, mangled_name, ctypes.byref(handle), ctypes.byref(outName), ctypes.byref(outSize), None
	    )
	):
		for i in range(outSize.value):
			names.append(outName[i].decode('utf8'))  # type: ignore
		core.BNFreeDemangledName(ctypes.byref(outName), outSize.value)
		if not handle:
			return (None, names)
		return (types.Type.create(core.BNNewTypeReference(handle)), names)
	return (None, mangled_name)


def simplify_name_to_string(input_name):
	"""
	``simplify_name_to_string`` simplifies a templated C++ name with default arguments and returns a string

	:param input_name: String or qualified name to be simplified
	:type input_name: Union[str, QualifiedName]
	:return: simplified name (or original name if simplifier fails/cannot simplify)
	:rtype: str
	:Example:

		>>> demangle.simplify_name_to_string("std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >")
		'std::string'
		>>>
	"""
	result = None
	if isinstance(input_name, str):
		result = core.BNRustSimplifyStrToStr(input_name)
	elif isinstance(input_name, types.QualifiedName):
		result = core.BNRustSimplifyStrToStr(str(input_name))
	else:
		raise TypeError("Parameter must be of type `str` or `types.QualifiedName`")
	return result


def simplify_name_to_qualified_name(input_name, simplify=True):
	"""
	``simplify_name_to_qualified_name`` simplifies a templated C++ name with default arguments and returns a qualified name.  This can also tokenize a string to a qualified name with/without simplifying it

	:param input_name: String or qualified name to be simplified
	:type input_name: Union[str, QualifiedName]
	:param bool simplify: (optional) Whether to simplify input string (no effect if given a qualified name; will always simplify)
	:return: simplified name (or one-element array containing the input if simplifier fails/cannot simplify)
	:rtype: QualifiedName
	:Example:

		>>> demangle.simplify_name_to_qualified_name(QualifiedName(["std", "__cxx11", "basic_string<wchar, std::char_traits<wchar>, std::allocator<wchar> >"]), True)
		'std::wstring'
		>>>
	"""
	result = None
	if isinstance(input_name, str):
		result = core.BNRustSimplifyStrToFQN(input_name, simplify)
		assert result is not None, "core.BNRustSimplifyStrToFQN returned None"
	elif isinstance(input_name, types.QualifiedName):
		result = core.BNRustSimplifyStrToFQN(str(input_name), True)
		assert result is not None, "core.BNRustSimplifyStrToFQN returned None"
	else:
		raise TypeError("Parameter must be of type `str` or `types.QualifiedName`")

	native_result = []
	for name in result:
		if name == b'':
			break
		native_result.append(name.decode("utf-8"))
	name_count = len(native_result)

	native_result = types.QualifiedName(native_result)
	core.BNRustFreeStringArray(result, name_count + 1)
	return native_result


class _DemanglerMetaclass(type):
	def __iter__(self):
		binaryninja._init_plugins()
		count = ctypes.c_ulonglong()
		types = core.BNGetDemanglerList(count)
		try:
			for i in range(0, count.value):
				yield CoreDemangler(types[i])
		finally:
			core.BNFreeDemanglerList(types)

	def __getitem__(self, value):
		binaryninja._init_plugins()
		handle = core.BNGetDemanglerByName(str(value))
		if handle is None:
			raise KeyError(f"'{value}' is not a valid Demangler")
		return CoreDemangler(handle)


class Demangler(metaclass=_DemanglerMetaclass):
	name = None
	_register_demanglers = []
	_cached_name = None

	def __init__(self, handle=None):
		if handle is not None:
			self.handle = core.handle_of_type(handle, core.BNDemangler)
			self.__dict__["name"] = core.BNGetDemanglerName(handle)

	def register(self):
		assert self.__class__.name is not None
		assert self.handle is None

		self._cb = core.BNDemanglerCallbacks()
		self._cb.context = 0
		self._cb.isMangledString = self._cb.isMangledString.__class__(self._is_mangled_string)
		self._cb.demangle = self._cb.demangle.__class__(self._demangle)
		self._cb.freeVarName = self._cb.freeVarName.__class__(self._free_var_name)
		self.handle = core.BNRegisterDemangler(self.__class__.name, self._cb)
		self.__class__._register_demanglers.append(self)

	def __str__(self):
		return f'<Demangler: {self.name}>'

	def __repr_(self):
		return f'<Demangler: {self.name}>'

	def _is_mangled_string(self, ctxt, name):
		try:
			return self.is_mangled_string(name)
		except:
			log_error(traceback.format_exc())
			return False

	def _demangle(self, ctxt, arch, name, view, out_type, out_var_name, simplify):
		try:
			api_arch = CoreArchitecture._from_cache(arch)
			api_view = None
			if view is not None:
				api_view = binaryview.BinaryView(handle=view)

			result = self.demangle(api_arch, name, api_view, simplify)
			if result is None:
				return False
			type, var_name = result

			Demangler._cached_name = var_name._to_core_struct()
			if type:
				out_type[0] = type.handle
			else:
				out_type[0] = None
			out_var_name[0] = Demangler._cached_name
			return True
		except:
			log_error(traceback.format_exc())
			return False

	def _free_var_name(self, ctxt, name):
		try:
			Demangler._cached_name = None
		except:
			log_error(traceback.format_exc())

	def is_mangled_string(self, name: str) -> bool:
		raise NotImplementedError()

	def demangle(self, arch: Architecture, name: str, view: Optional['binaryview.BinaryView'] = None,
				 simplify: bool = False) -> Optional[Tuple['types.Type', 'types.QualifiedName']]:
		raise NotImplementedError()


class CoreDemangler(Demangler):

	def is_mangled_string(self, name: str) -> bool:
		return core.BNIsDemanglerMangledName(self.handle, name)

	def demangle(self, arch: Architecture, name: str, view: Optional['binaryview.BinaryView'] = None,
				 simplify: bool = False) -> Optional[Tuple[Optional['types.Type'], 'types.QualifiedName']]:
		out_type = ctypes.POINTER(core.BNType)()
		out_var_name = core.BNQualifiedName()

		view_handle = None
		if view is not None:
			view_handle = view.handle

		if not core.BNDemanglerDemangle(self.handle, arch.handle, name, out_type, out_var_name, view_handle, simplify):
			return None

		if out_type.contents:
			result_type = types.Type(handle=out_type)
		result_var_name = types.QualifiedName._from_core_struct(out_var_name)
		return result_type, result_var_name
