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
from . import types as ty_
from . import metadata
from . import platform
from . import architecture


class TypeArchive:
	def __init__(self, handle: core.BNTypeArchiveHandle):
		binaryninja._init_plugins()
		self.handle: core.BNTypeArchiveHandle = core.handle_of_type(handle, core.BNTypeArchive)

	def __hash__(self):
		return hash(ctypes.addressof(self.handle.contents))

	def __del__(self):
		if core is not None:
			core.BNFreeTypeArchiveReference(self.handle)

	def __repr__(self):
		return f"<type archive '{self.path}'>"

	def __eq__(self, other):
		if not isinstance(other, TypeArchive):
			return False
		return self.id == other.id

	@staticmethod
	def open(path: str) -> Optional['TypeArchive']:
		"""

		:param path:
		:return:
		"""
		handle = core.BNOpenTypeArchive(path)
		if handle is None:
			return None
		return TypeArchive(handle=handle)

	@staticmethod
	def lookup_by_id(id: str) -> Optional['TypeArchive']:
		"""

		:param id:
		:return:
		"""
		handle = core.BNLookupTypeArchiveById(id)
		if handle is None:
			return None
		return TypeArchive(handle=handle)

	@property
	def path(self) -> Optional[str]:
		"""

		:return:
		"""
		return core.BNGetTypeArchivePath(self.handle)

	@property
	def id(self) -> Optional[str]:
		"""

		:return:
		"""
		return core.BNGetTypeArchiveId(self.handle)

	@property
	def current_snapshot_id(self) -> str:
		"""

		:return:
		"""
		result = core.BNGetTypeArchiveCurrentSnapshotId(self.handle)
		if result is None:
			raise RuntimeError("BNGetTypeArchiveCurrentSnapshotId")
		return result

	@property
	def all_snapshot_ids(self) -> List[str]:
		"""

		:return:
		"""
		count = ctypes.c_ulonglong(0)
		ids = core.BNGetTypeArchiveAllSnapshotIds(self.handle, count)
		if ids is None:
			raise RuntimeError("BNGetTypeArchiveAllSnapshotIds")
		result = []
		try:
			for i in range(0, count.value):
				result.append(core.pyNativeStr(ids[i]))
			return result
		finally:
			core.BNFreeStringList(ids, count.value)

	def get_snapshot_parent_id(self, snapshot: str) -> Optional[str]:
		"""

		:param snapshot:
		:return:
		"""
		result = core.BNGetTypeArchiveSnapshotParentId(self.handle, snapshot)
		if result is None:
			raise RuntimeError("BNGetTypeArchiveSnapshotParentId")
		if result == "":
			return None
		return result

	def add_type(self, name: 'ty_.QualifiedNameType', type: 'ty_.Type') -> None:
		"""

		:param QualifiedName name:
		:param Type t:
		:rtype: None
		"""
		self.add_types([(name, type)])

	def add_types(self, new_types: List[Tuple['ty_.QualifiedNameType', 'ty_.Type']]) -> None:
		"""

		:param new_types:
		"""
		api_types = (core.BNQualifiedNameAndType * len(new_types))()
		i = 0
		for (name, type) in new_types:
			if not isinstance(name, ty_.QualifiedName):
				name = ty_.QualifiedName(name)
			type = type.immutable_copy()
			if not isinstance(type, ty_.Type):
				raise ValueError("parameter type must be a Type")
			api_types[i].name = name._to_core_struct()
			api_types[i].type = type.handle
			i += 1

		if not core.BNAddTypeArchiveTypes(self.handle, api_types, len(new_types)):
			raise RuntimeError("BNAddTypeArchiveTypes")

	def rename_type(self, old_name: 'ty_.QualifiedNameType', new_name: 'ty_.QualifiedNameType') -> None:
		"""

		:param old_name:
		:param new_name:
		"""
		id = self.get_type_id(old_name)
		return self.rename_type_by_id(id, new_name)

	def rename_type_by_id(self, id: str, new_name: 'ty_.QualifiedNameType') -> None:
		"""

		:param id:
		:param new_name:
		"""
		if not isinstance(new_name, ty_.QualifiedName):
			new_name = ty_.QualifiedName(new_name)
		if not core.BNRenameTypeArchiveType(self.handle, id, new_name._to_core_struct()):
			raise RuntimeError("BNRenameTypeArchiveType")

	def remove_type(self, name: 'ty_.QualifiedNameType') -> None:
		"""

		:param name:
		"""
		id = self.get_type_id(name)
		if id is None:
			raise RuntimeError(f"Unknown type {name}")
		self.remove_type_by_id(id)

	def remove_type_by_id(self, id: str) -> None:
		"""

		:param id:
		"""
		if not core.BNRemoveTypeArchiveType(self.handle, id):
			raise RuntimeError("BNRemoveTypeArchiveType")

	def get_type_by_name(self, name: 'ty_.QualifiedNameType', snapshot: Optional[str] = None) -> Optional[ty_.Type]:
		"""

		:param QualifiedName name:
		:rtype: Type
		"""
		if snapshot is None:
			snapshot = self.current_snapshot_id
		if not isinstance(name, ty_.QualifiedName):
			name = ty_.QualifiedName(name)
		t = core.BNGetTypeArchiveTypeByName(self.handle, name._to_core_struct(), snapshot)
		if t is None:
			return None
		return ty_.Type.create(t)

	def get_type_by_id(self, id: str, snapshot: Optional[str] = None) -> Optional[ty_.Type]:
		"""

		:param str id:
		:rtype: Type
		"""
		if snapshot is None:
			snapshot = self.current_snapshot_id
		assert id is not None
		t = core.BNGetTypeArchiveTypeById(self.handle, id, snapshot)
		if t is None:
			return None
		return ty_.Type.create(t)

	def get_type_name(self, id: str, snapshot: Optional[str] = None) -> Optional['ty_.QualifiedName']:
		"""

		:param str id:
		:rtype: Type
		"""
		if snapshot is None:
			snapshot = self.current_snapshot_id
		assert id is not None
		name = core.BNGetTypeArchiveTypeName(self.handle, id, snapshot)
		if name is None:
			return None
		qname = ty_.QualifiedName._from_core_struct(name)
		if len(qname.name) == 0:
			return None
		return qname

	def get_type_id(self, name: 'ty_.QualifiedNameType', snapshot: Optional[str] = None) -> Optional[str]:
		"""

		:param QualifiedName name:
		:rtype: Type
		"""
		if snapshot is None:
			snapshot = self.current_snapshot_id
		if not isinstance(name, ty_.QualifiedName):
			name = ty_.QualifiedName(name)
		t = core.BNGetTypeArchiveTypeId(self.handle, name._to_core_struct(), snapshot)
		if t is None:
			return None
		if t == "":
			return None
		return t

	@property
	def types(self) -> Dict[ty_.QualifiedName, ty_.Type]:
		"""

		:return:
		"""
		return self.get_types()

	@property
	def types_and_ids(self) -> Dict[str, Tuple[ty_.QualifiedName, ty_.Type]]:
		"""

		:return:
		"""
		return self.get_types_and_ids()

	def get_types(self, snapshot: Optional[str] = None) -> Dict[ty_.QualifiedName, ty_.Type]:
		"""

		:param snapshot:
		:return:
		"""
		result = {}
		for id, (name, type) in self.get_types_and_ids(snapshot).items():
			result[name] = type
		return result

	def get_types_and_ids(self, snapshot: Optional[str] = None) -> Dict[str, Tuple[ty_.QualifiedName, ty_.Type]]:
		"""

		:param snapshot:
		:return:
		"""
		if snapshot is None:
			snapshot = self.current_snapshot_id
		count = ctypes.c_ulonglong(0)
		result = {}
		named_types = core.BNGetTypeArchiveTypes(self.handle, snapshot, count)
		assert named_types is not None, "core.BNGetTypeArchiveTypes returned None"
		try:
			for i in range(0, count.value):
				name = ty_.QualifiedName._from_core_struct(named_types[i].name)
				id = core.pyNativeStr(named_types[i].id)
				result[id] = (name, ty_.Type.create(core.BNNewTypeReference(named_types[i].type)))
			return result
		finally:
			core.BNFreeTypeIdList(named_types, count.value)

	@property
	def type_ids(self) -> List[str]:
		"""

		:return:
		"""
		return self.get_type_ids()

	def get_type_ids(self, snapshot: Optional[str] = None) -> List[str]:
		"""

		:param snapshot:
		:return:
		"""
		if snapshot is None:
			snapshot = self.current_snapshot_id
		count = ctypes.c_ulonglong(0)
		result = []
		ids = core.BNGetTypeArchiveTypeIds(self.handle, snapshot, count)
		assert ids is not None, "core.BNGetTypeArchiveTypeIds returned None"
		try:
			for i in range(count.value):
				result.append(core.pyNativeStr(ids[i]))
			return result
		finally:
			core.BNFreeStringList(ids, count.value)

	@property
	def type_names(self) -> List['ty_.QualifiedName']:
		"""

		:return:
		"""
		return self.get_type_names()

	def get_type_names(self, snapshot: Optional[str] = None) -> List['ty_.QualifiedName']:
		"""

		:param snapshot:
		:return:
		"""
		if snapshot is None:
			snapshot = self.current_snapshot_id
		count = ctypes.c_ulonglong(0)
		result = []
		names = core.BNGetTypeArchiveTypeNames(self.handle, snapshot, count)
		assert names is not None, "core.BNGetTypeArchiveTypeNames returned None"
		try:
			for i in range(count.value):
				result.append(ty_.QualifiedName._from_core_struct(names[i]))
			return result
		finally:
			core.BNFreeQualifiedNameArray(names, count.value)

	@property
	def type_names_and_ids(self) -> Dict[str, 'ty_.QualifiedName']:
		"""

		:return:
		"""
		return self.get_type_names_and_ids()

	def get_type_names_and_ids(self, snapshot: Optional[str] = None) -> Dict[str, 'ty_.QualifiedName']:
		"""

		:param snapshot:
		:return:
		"""
		if snapshot is None:
			snapshot = self.current_snapshot_id
		names = ctypes.POINTER(core.BNQualifiedName)()
		ids = ctypes.POINTER(ctypes.c_char_p)()
		count = ctypes.c_ulonglong(0)
		result = {}
		if not core.BNGetTypeArchiveTypeNamesAndIds(self.handle, snapshot, names, ids, count):
			raise RuntimeError("core.BNGetTypeArchiveTypeNamesAndIds returned False")
		try:
			for i in range(count.value):
				id = core.pyNativeStr(ids[i])
				name = ty_.QualifiedName._from_core_struct(names[i])
				result[id] = name
			return result
		finally:
			core.BNFreeQualifiedNameArray(names, count.value)

	def get_outgoing_direct_references(self, id: str, snapshot: Optional[str] = None) -> List[str]:
		"""

		:param id:
		:param snapshot:
		:return:
		"""
		if snapshot is None:
			snapshot = self.current_snapshot_id
		assert id is not None
		count = ctypes.c_size_t(0)
		ids = core.BNGetTypeArchiveOutgoingDirectTypeReferences(self.handle, id, snapshot, count)
		if ids is None:
			raise RuntimeError("BNGetTypeArchiveOutgoingDirectTypeReferences")
		result = []
		try:
			for i in range(0, count.value):
				result.append(core.pyNativeStr(ids[i]))
			return result
		finally:
			core.BNFreeStringList(ids, count.value)

	def get_outgoing_recursive_references(self, id: str, snapshot: Optional[str] = None) -> List[str]:
		"""

		:param id:
		:param snapshot:
		:return:
		"""
		if snapshot is None:
			snapshot = self.current_snapshot_id
		assert id is not None
		count = ctypes.c_size_t(0)
		ids = core.BNGetTypeArchiveOutgoingRecursiveTypeReferences(self.handle, id, snapshot, count)
		if ids is None:
			raise RuntimeError("BNGetTypeArchiveOutgoingRecursiveTypeReferences")
		result = []
		try:
			for i in range(0, count.value):
				result.append(core.pyNativeStr(ids[i]))
			return result
		finally:
			core.BNFreeStringList(ids, count.value)

	def get_incoming_direct_references(self, id: str, snapshot: Optional[str] = None) -> List[str]:
		"""

		:param id:
		:param snapshot:
		:return:
		"""
		if snapshot is None:
			snapshot = self.current_snapshot_id
		assert id is not None
		count = ctypes.c_size_t(0)
		ids = core.BNGetTypeArchiveIncomingDirectTypeReferences(self.handle, id, snapshot, count)
		if ids is None:
			raise RuntimeError("BNGetTypeArchiveIncomingDirectTypeReferences")
		result = []
		try:
			for i in range(0, count.value):
				result.append(core.pyNativeStr(ids[i]))
			return result
		finally:
			core.BNFreeStringList(ids, count.value)

	def get_incoming_recursive_references(self, id: str, snapshot: Optional[str] = None) -> List[str]:
		"""

		:param id:
		:param snapshot:
		:return:
		"""
		if snapshot is None:
			snapshot = self.current_snapshot_id
		assert id is not None
		count = ctypes.c_size_t(0)
		ids = core.BNGetTypeArchiveIncomingRecursiveTypeReferences(self.handle, id, snapshot, count)
		if ids is None:
			raise RuntimeError("BNGetTypeArchiveIncomingRecursiveTypeReferences")
		result = []
		try:
			for i in range(0, count.value):
				result.append(core.pyNativeStr(ids[i]))
			return result
		finally:
			core.BNFreeStringList(ids, count.value)


	def query_metadata(self, key: str) -> Optional['metadata.MetadataValueType']:
		"""

		:param string key: key to query
		:rtype: metadata associated with the key
		:Example:

			>>> lib.store_metadata("ordinals", {"9": "htons"})
			>>> lib.query_metadata("ordinals")["9"]
			"htons"
		"""
		md_handle = core.BNTypeArchiveQueryMetadata(self.handle, key)
		if md_handle is None:
			return None
		return metadata.Metadata(handle=md_handle).value

	def store_metadata(self, key: str, md: 'metadata.MetadataValueType') -> None:
		"""

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
		core.BNTypeArchiveStoreMetadata(self.handle, key, md.handle)

	def remove_metadata(self, key: str) -> None:
		"""

		:param string key: key associated with metadata
		:rtype: None
		:Example:

			>>> lib.store_metadata("integer", 1337)
			>>> lib.remove_metadata("integer")
		"""
		core.BNTypeArchiveRemoveMetadata(self.handle, key)
