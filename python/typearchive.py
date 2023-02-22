# Copyright (c) 2015-2023 Vector 35 Inc
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
from typing import Optional, List, Dict, Union, Tuple

# Binary Ninja components
import binaryninja
from . import _binaryninjacore as core
from . import types
from . import metadata
from . import platform
from . import architecture


class TypeArchive:
	def __init__(self, handle: core.BNTypeArchiveHandle):
		binaryninja._init_plugins()
		self.handle: core.BNTypeArchiveHandle = core.handle_of_type(handle, core.BNTypeArchive)

	def __del__(self):
		if core is not None:
			core.BNFreeTypeArchiveReference(self.handle)

	def __repr__(self):
		return f"<type archive '{self.path}'>"

	@staticmethod
	def open(path: str) -> Optional['TypeArchive']:
		handle = core.BNOpenTypeArchive(path)
		if handle is None:
			return None
		return TypeArchive(handle=handle)

	@property
	def path(self) -> Optional[str]:
		return core.BNGetTypeArchivePath(self.handle)

	@property
	def id(self) -> Optional[str]:
		return core.BNGetTypeArchiveId(self.handle)

	def query_metadata(self, key: str) -> Optional[metadata.Metadata]:
		"""
		`query_metadata` retrieves a metadata associated with the given key stored in the type library

		:param string key: key to query
		:rtype: metadata associated with the key
		:Example:

			>>> lib.store_metadata("ordinals", {"9": "htons"})
			>>> lib.query_metadata("ordinals")["9"]
			"htons"
		"""
		md_handle = core.BNQueryTypeArchiveMetadata(self.handle, key)
		if md_handle is None:
			return None
		return metadata.Metadata(handle=md_handle).value

	def store_metadata(self, key: str, md: metadata.Metadata) -> None:
		"""
		`store_metadata` stores an object for the given key in the current type library. Objects stored using
		`store_metadata` can be retrieved from any reference to the library. Objects stored are not arbitrary python
		objects! The values stored must be able to be held in a Metadata object. See :py:class:`Metadata`
		for more information. Python objects could obviously be serialized using pickle but this intentionally
		a task left to the user since there is the potential security issues.

		This is primarily intended as a way to store Platform specific information relevant to BinaryView implementations;
		for example the PE BinaryViewType uses type library metadata to retrieve ordinal information, when available.

		:param string key: key value to associate the Metadata object with
		:param Varies md: object to store.
		:rtype: None
		:Example:

			>>> lib.store_metadata("ordinals", {"9": "htons"})
			>>> lib.query_metadata("ordinals")["9"]
			"htons"
		"""
		if not isinstance(md, metadata.Metadata):
			md = metadata.Metadata(md)
		core.BNStoreTypeArchiveMetadata(self.handle, key, md.handle)

	def remove_metadata(self, key: str) -> None:
		"""
		`remove_metadata` removes the metadata associated with key from the current type library.

		:param string key: key associated with metadata
		:rtype: None
		:Example:

			>>> lib.store_metadata("integer", 1337)
			>>> lib.remove_metadata("integer")
		"""
		core.BNRemoveTypeArchiveMetadata(self.handle, key)

	def add_named_type(self, name: 'types.QualifiedNameType', type: 'types.Type') -> None:
		"""
		`add_named_type` directly inserts a named object into the type library's object store.
		This is not done recursively, so care should be taken that types referring to other types
		through NamedTypeReferences are already appropriately prepared.

		To add types and objects from an existing BinaryView, it is recommended to use
		:py:meth:`export_type_to_library <binaryview.BinaryView.export_type_to_library>`, which will automatically pull in
		all referenced types and record additional dependencies as needed.

		:param QualifiedName name:
		:param Type t:
		:rtype: None
		"""
		self.add_named_types([(name, type)])

	def add_named_types(self, new_types: List[Tuple['types.QualifiedNameType', 'types.Type']]) -> None:
		api_types = (core.BNQualifiedNameAndType * len(new_types))()
		i = 0
		for (name, type) in new_types:
			if not isinstance(name, types.QualifiedName):
				name = types.QualifiedName(name)
			type = type.immutable_copy()
			if not isinstance(type, types.Type):
				raise ValueError("parameter type must be a Type")
			api_types[i].name = name._to_core_struct()
			api_types[i].type = type.handle
			i += 1

		if not core.BNAddTypeArchiveNamedTypes(self.handle, api_types, len(new_types)):
			raise RuntimeError("BNAddTypeArchiveNamedTypes")

	def get_type_by_name(self, name: Union[str, types.QualifiedName]) -> Optional[types.Type]:
		"""
		`get_type_by_name` direct extracts a reference to a contained type -- when
		attempting to extract types from a library into a BinaryView, consider using
		:py:meth:`import_library_type <binaryview.BinaryView.import_library_type>` instead.

		:param QualifiedName name:
		:rtype: Type
		"""
		if not isinstance(name, types.QualifiedName):
			name = types.QualifiedName(name)
		t = core.BNGetTypeArchiveTypeByName(self.handle, name._to_core_struct())
		if t is None:
			return None
		return types.Type.create(t)

	def get_type_by_id(self, id: str) -> Optional[types.Type]:
		"""
		`get_type_by_id` direct extracts a reference to a contained type -- when
		attempting to extract types from a library into a BinaryView, consider using
		:py:meth:`import_library_type <binaryview.BinaryView.import_library_type>` instead.

		:param str id:
		:rtype: Type
		"""
		t = core.BNGetTypeArchiveTypeById(self.handle, id)
		if t is None:
			return None
		return types.Type.create(t)

	def get_type_name(self, id: str) -> Optional['types.QualifiedName']:
		"""
		`get_type_by_id` direct extracts a reference to a contained type -- when
		attempting to extract types from a library into a BinaryView, consider using
		:py:meth:`import_library_type <binaryview.BinaryView.import_library_type>` instead.

		:param str id:
		:rtype: Type
		"""
		name = core.BNGetTypeArchiveTypeName(self.handle, id)
		if name is None:
			return None
		qname = types.QualifiedName._from_core_struct(name)
		if len(qname.name) == 0:
			return None
		return qname

	def get_type_id(self, name: Union[str, types.QualifiedName]) -> Optional[str]:
		"""
		`get_type_by_name` direct extracts a reference to a contained type -- when
		attempting to extract types from a library into a BinaryView, consider using
		:py:meth:`import_library_type <binaryview.BinaryView.import_library_type>` instead.

		:param QualifiedName name:
		:rtype: Type
		"""
		if not isinstance(name, types.QualifiedName):
			name = types.QualifiedName(name)
		t = core.BNGetTypeArchiveTypeId(self.handle, name._to_core_struct())
		if t is None:
			return None
		if t == "":
			return None
		return t

	@property
	def named_types(self) -> Dict[types.QualifiedName, types.Type]:
		"""
		A dict containing all named types provided by a type library (read-only)
		"""
		count = ctypes.c_ulonglong(0)
		result = {}
		named_types = core.BNGetTypeArchiveNamedTypes(self.handle, count)
		assert named_types is not None, "core.BNGetTypeArchiveNamedTypes returned None"
		try:
			for i in range(0, count.value):
				name = types.QualifiedName._from_core_struct(named_types[i].name)
				result[name] = types.Type.create(core.BNNewTypeReference(named_types[i].type))
			return result
		finally:
			core.BNFreeQualifiedNameAndTypeArray(named_types, count.value)

	def get_direct_references(self, id: str) -> List[str]:
		"""

		:param id:
		:return:
		"""
		count = ctypes.c_size_t(0)
		ids = core.BNGetTypeArchiveDirectTypeReferences(self.handle, id, count)
		if ids is None:
			raise RuntimeError("BNGetTypeArchiveDirectTypeReferences")
		result = []
		try:
			for i in range(0, count.value):
				result.append(ids[i])
			return result
		finally:
			core.BNFreeStringList(ids, count.value)

	def get_recursive_references(self, id: str) -> List[str]:
		"""

		:param id:
		:return:
		"""
		count = ctypes.c_size_t(0)
		ids = core.BNGetTypeArchiveRecursiveTypeReferences(self.handle, id, count)
		if ids is None:
			raise RuntimeError("BNGetTypeArchiveRecursiveTypeReferences")
		result = []
		try:
			for i in range(0, count.value):
				result.append(ids[i])
			return result
		finally:
			core.BNFreeStringList(ids, count.value)

	def get_incoming_direct_references(self, id: str) -> List[str]:
		"""

		:param id:
		:return:
		"""
		count = ctypes.c_size_t(0)
		ids = core.BNGetTypeArchiveIncomingDirectTypeReferences(self.handle, id, count)
		if ids is None:
			raise RuntimeError("BNGetTypeArchiveIncomingDirectTypeReferences")
		result = []
		try:
			for i in range(0, count.value):
				result.append(ids[i])
			return result
		finally:
			core.BNFreeStringList(ids, count.value)

