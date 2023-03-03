
#include "binaryninjaapi.h"

using namespace BinaryNinja;


void TypeArchiveNotification::OnViewConnectedCallback(void* ctx, BNBinaryView* view)
{
	TypeArchiveNotification* notify = reinterpret_cast<TypeArchiveNotification*>(ctx);
	Ref<BinaryView> cppView = new BinaryView(BNNewViewReference(view));
	notify->OnViewConnected(cppView);
}


void TypeArchiveNotification::OnViewDisconnectedCallback(void* ctx, BNBinaryView* view)
{
	TypeArchiveNotification* notify = reinterpret_cast<TypeArchiveNotification*>(ctx);
	Ref<BinaryView> cppView = new BinaryView(BNNewViewReference(view));
	notify->OnViewDisconnected(cppView);
}


void TypeArchiveNotification::OnTypeAddedCallback(void* ctx, const char* id, BNType* definition)
{
	TypeArchiveNotification* notify = reinterpret_cast<TypeArchiveNotification*>(ctx);
	Ref<Type> cppDefinition = new Type(BNNewTypeReference(definition));
	notify->OnTypeAdded(id, cppDefinition);
}


void TypeArchiveNotification::OnTypeUpdatedCallback(void* ctx, const char* id, BNType* oldDefinition, BNType* newDefinition)
{
	TypeArchiveNotification* notify = reinterpret_cast<TypeArchiveNotification*>(ctx);
	Ref<Type> cppOldDefinition = new Type(BNNewTypeReference(oldDefinition));
	Ref<Type> cppNewDefinition = new Type(BNNewTypeReference(newDefinition));
	notify->OnTypeUpdated(id, cppOldDefinition, cppNewDefinition);
}


void TypeArchiveNotification::OnTypeRenamedCallback(void* ctx, const char* id, const BNQualifiedName* oldName, const BNQualifiedName* newName)
{
	TypeArchiveNotification* notify = reinterpret_cast<TypeArchiveNotification*>(ctx);
	QualifiedName appOldName = QualifiedName::FromAPIObject(oldName);
	QualifiedName appNewName = QualifiedName::FromAPIObject(newName);
	notify->OnTypeRenamed(id, oldName, newName);
}


void TypeArchiveNotification::OnTypeDeletedCallback(void* ctx, const char* id, BNType* definition)
{
	TypeArchiveNotification* notify = reinterpret_cast<TypeArchiveNotification*>(ctx);
	Ref<Type> cppDefinition = new Type(BNNewTypeReference(definition));
	notify->OnTypeDeleted(id, cppDefinition);
}


TypeArchiveNotification::TypeArchiveNotification()
{
	m_callbacks.context = this;
	m_callbacks.viewConnected = OnViewConnectedCallback;
	m_callbacks.viewDisconnected = OnViewDisconnectedCallback;
	m_callbacks.typeAdded = OnTypeAddedCallback;
	m_callbacks.typeUpdated = OnTypeUpdatedCallback;
	m_callbacks.typeRenamed = OnTypeRenamedCallback;
	m_callbacks.typeDeleted = OnTypeDeletedCallback;
}


TypeArchive::TypeArchive(BNTypeArchive* archive)
{
	m_object = archive;
}


Ref<TypeArchive> TypeArchive::Open(const std::string& path)
{
	BNTypeArchive* handle = BNOpenTypeArchive(path.c_str());
	if (!handle)
		return nullptr;
	return new TypeArchive(handle);
}


Ref<TypeArchive> TypeArchive::LookupById(const std::string& id)
{
	BNTypeArchive* handle = BNLookupTypeArchiveById(id.c_str());
	if (!handle)
		return nullptr;
	return new TypeArchive(handle);
}


std::string TypeArchive::GetId() const
{
	char* str = BNGetTypeArchiveId(m_object);
	std::string result(str);
	BNFreeString(str);
	return result;
}


std::string TypeArchive::GetPath() const
{
	char* str = BNGetTypeArchivePath(m_object);
	std::string result(str);
	BNFreeString(str);
	return result;
}


std::string TypeArchive::GetCurrentSnapshotId() const noexcept(false)
{
	char* str = BNGetTypeArchiveCurrentSnapshotId(m_object);
	if (!str)
		throw DatabaseException("BNGetTypeArchiveCurrentSnapshotId");
	std::string result(str);
	BNFreeString(str);
	return result;
}


std::vector<std::string> TypeArchive::GetAllSnapshotIds() const noexcept(false)
{
	size_t count = 0;
	char** ids = BNGetTypeArchiveAllSnapshotIds(m_object, &count);
	if (!ids)
		throw DatabaseException("BNGetTypeArchiveAllSnapshotIds");

	std::vector<std::string> result;
	for (size_t i = 0; i < count; i ++)
	{
		result.push_back(ids[i]);
	}

	BNFreeStringList(ids, count);
	return result;
}


std::string TypeArchive::GetSnapshotParentId(const std::string& id) const noexcept(false)
{
	char* str = BNGetTypeArchiveSnapshotParentId(m_object, id.c_str());
	if (!str)
		throw DatabaseException("BNGetTypeArchiveSnapshotParentId");
	std::string result(str);
	BNFreeString(str);
	return result;
}


bool TypeArchive::AddTypes(const std::vector<QualifiedNameAndType>& types) noexcept(false)
{
	std::vector<BNQualifiedNameAndType> apiTypes;
	for (auto& type : types)
	{
		BNQualifiedNameAndType qnat;
		qnat.name = type.name.GetAPIObject();
		qnat.type = type.type->GetObject();
		apiTypes.push_back(qnat);
	}
	bool result = BNAddTypeArchiveTypes(m_object, apiTypes.data(), apiTypes.size());
	for (auto& type: apiTypes)
	{
		QualifiedName::FreeAPIObject(&type.name);
	}
	return result;
}


bool TypeArchive::RenameType(const std::string& id, const QualifiedName& newName) noexcept(false)
{
	BNQualifiedName qname = newName.GetAPIObject();
	bool result = BNRenameTypeArchiveType(m_object, id.c_str(), &qname);
	QualifiedName::FreeAPIObject(&qname);
	return result;
}


bool TypeArchive::RemoveType(const std::string& id) noexcept(false)
{
	return BNRemoveTypeArchiveType(m_object, id.c_str());
}


Ref<Type> TypeArchive::GetTypeById(const std::string& id, std::string snapshot) const noexcept(false)
{
	if (snapshot.empty())
		snapshot = GetCurrentSnapshotId();
	BNType* type = BNGetTypeArchiveTypeById(m_object, id.c_str(), snapshot.c_str());
	if (!type)
		return nullptr;
	return new Type(type);
}


Ref<Type> TypeArchive::GetTypeByName(const QualifiedName& name, std::string snapshot) const noexcept(false)
{
	if (snapshot.empty())
		snapshot = GetCurrentSnapshotId();
	BNQualifiedName qname = name.GetAPIObject();
	BNType* type = BNGetTypeArchiveTypeByName(m_object, &qname, snapshot.c_str());
	QualifiedName::FreeAPIObject(&qname);
	if (!type)
		return nullptr;
	return new Type(type);
}


std::string TypeArchive::GetTypeId(const QualifiedName& name, std::string snapshot) const noexcept(false)
{
	if (snapshot.empty())
		snapshot = GetCurrentSnapshotId();
	BNQualifiedName qname = name.GetAPIObject();
	char* id = BNGetTypeArchiveTypeId(m_object, &qname, snapshot.c_str());
	QualifiedName::FreeAPIObject(&qname);
	if (!id)
		return "";
	std::string result = id;
	BNFreeString(id);
	return result;

}


QualifiedName TypeArchive::GetTypeName(const std::string& id, std::string snapshot) const noexcept(false)
{
	if (snapshot.empty())
		snapshot = GetCurrentSnapshotId();
	BNQualifiedName qname = BNGetTypeArchiveTypeName(m_object, id.c_str(), snapshot.c_str());
	QualifiedName result = QualifiedName::FromAPIObject(&qname);
	BNFreeQualifiedName(&qname);
	return result;
}


std::unordered_map<std::string, QualifiedNameAndType> TypeArchive::GetTypes(std::string snapshot) const noexcept(false)
{
	if (snapshot.empty())
	snapshot = GetCurrentSnapshotId();
	size_t count = 0;
	BNQualifiedNameTypeAndId* types = BNGetTypeArchiveTypes(m_object, snapshot.c_str(), &count);
	if (!types)
		throw DatabaseException("BNGetTypeArchiveTypes");

	std::unordered_map<std::string, QualifiedNameAndType> result;
	for (size_t i = 0; i < count; ++i)
	{
		std::string id = types[i].id;
		QualifiedNameAndType qnat;
		qnat.name = QualifiedName::FromAPIObject(&types[i].name);
		qnat.type = new Type(types[i].type);
		result.emplace(id, qnat);
	}
	BNFreeTypeIdList(types, count);
	return result;
}


std::vector<std::string> TypeArchive::GetTypeIds(std::string snapshot) const noexcept(false)
{
	if (snapshot.empty())
		snapshot = GetCurrentSnapshotId();
	size_t count = 0;
	char** ids = BNGetTypeArchiveTypeIds(m_object, snapshot.c_str(), &count);
	if (!ids)
		throw DatabaseException("BNGetTypeArchiveTypeIds");

	std::vector<std::string> result;
	for (size_t i = 0; i < count; ++i)
	{
		result.push_back(ids[i]);
	}
	BNFreeStringList(ids, count);
	return result;
}


std::vector<QualifiedName> TypeArchive::GetTypeNames(std::string snapshot) const noexcept(false)
{
	if (snapshot.empty())
		snapshot = GetCurrentSnapshotId();
	size_t count = 0;
	BNQualifiedName* names = BNGetTypeArchiveTypeNames(m_object, snapshot.c_str(), &count);
	if (!names)
		throw DatabaseException("BNGetTypeArchiveTypeNames");

	std::vector<QualifiedName> result;
	for (size_t i = 0; i < count; ++i)
	{
		result.push_back(QualifiedName::FromAPIObject(&names[i]));
	}
	BNFreeTypeNameList(names, count);
	return result;
}


std::unordered_map<std::string, QualifiedName> TypeArchive::GetTypeNamesAndIds(std::string snapshot) const noexcept(false)
{
	if (snapshot.empty())
		snapshot = GetCurrentSnapshotId();
	BNQualifiedName* names = nullptr;
	char** ids = nullptr;
	size_t count = 0;
	if (!BNGetTypeArchiveTypeNamesAndIds(m_object, snapshot.c_str(), &names, &ids, &count))
		throw DatabaseException("BNGetTypeArchiveTypeNamesAndIds");

	std::unordered_map<std::string, QualifiedName> result;
	for (size_t i = 0; i < count; ++i)
	{
		result.emplace(ids[i], QualifiedName::FromAPIObject(&names[i]));
	}
	BNFreeTypeNameList(names, count);
	BNFreeStringList(ids, count);
	return result;
}


std::unordered_set<std::string> TypeArchive::GetOutgoingDirectTypeReferences(const std::string& id, std::string snapshot) const noexcept(false)
{
	if (snapshot.empty())
		snapshot = GetCurrentSnapshotId();
	size_t count = 0;
	char** ids = BNGetTypeArchiveOutgoingDirectTypeReferences(m_object, id.c_str(), snapshot.c_str(), &count);
	if (!ids)
		throw DatabaseException("BNGetTypeArchiveOutgoingDirectTypeReferences");

	std::unordered_set<std::string> result;
	for (size_t i = 0; i < count; ++i)
	{
		result.insert(ids[i]);
	}
	BNFreeStringList(ids, count);
	return result;
}


std::unordered_set<std::string> TypeArchive::GetOutgoingRecursiveTypeReferences(const std::string& id, std::string snapshot) const noexcept(false)
{
	if (snapshot.empty())
		snapshot = GetCurrentSnapshotId();
	size_t count = 0;
	char** ids = BNGetTypeArchiveOutgoingRecursiveTypeReferences(m_object, id.c_str(), snapshot.c_str(), &count);
	if (!ids)
		throw DatabaseException("BNGetTypeArchiveOutgoingRecursiveTypeReferences");

	std::unordered_set<std::string> result;
	for (size_t i = 0; i < count; ++i)
	{
		result.insert(ids[i]);
	}
	BNFreeStringList(ids, count);
	return result;
}


std::unordered_set<std::string> TypeArchive::GetIncomingDirectTypeReferences(const std::string& id, std::string snapshot) const noexcept(false)
{
	if (snapshot.empty())
		snapshot = GetCurrentSnapshotId();
	size_t count = 0;
	char** ids = BNGetTypeArchiveIncomingDirectTypeReferences(m_object, id.c_str(), snapshot.c_str(), &count);
	if (!ids)
		throw DatabaseException("BNGetTypeArchiveIncomingDirectTypeReferences");

	std::unordered_set<std::string> result;
	for (size_t i = 0; i < count; ++i)
	{
		result.insert(ids[i]);
	}
	BNFreeStringList(ids, count);
	return result;
}


std::unordered_set<std::string> TypeArchive::GetIncomingRecursiveTypeReferences(const std::string& id, std::string snapshot) const noexcept(false)
{
	if (snapshot.empty())
		snapshot = GetCurrentSnapshotId();
	size_t count = 0;
	char** ids = BNGetTypeArchiveIncomingRecursiveTypeReferences(m_object, id.c_str(), snapshot.c_str(), &count);
	if (!ids)
		throw DatabaseException("BNGetTypeArchiveIncomingRecursiveTypeReferences");

	std::unordered_set<std::string> result;
	for (size_t i = 0; i < count; ++i)
	{
		result.insert(ids[i]);
	}
	BNFreeStringList(ids, count);
	return result;
}


void TypeArchive::RegisterNotification(TypeArchiveNotification* notification)
{
	BNRegisterTypeArchiveNotification(m_object, notification->GetCallbacks());
}


void TypeArchive::UnregisterNotification(TypeArchiveNotification* notification)
{
	BNUnregisterTypeArchiveNotification(m_object, notification->GetCallbacks());
}


void TypeArchive::StoreMetadata(const std::string& key, Ref<Metadata> value) noexcept(false)
{
	if (!BNTypeArchiveStoreMetadata(m_object, key.c_str(), value->GetObject()))
		throw DatabaseException("BNTypeArchiveStoreMetadata");
}


Ref<Metadata> TypeArchive::QueryMetadata(const std::string& key) const noexcept(false)
{
	BNMetadata* metadata = BNTypeArchiveQueryMetadata(m_object, key.c_str());
	if (!metadata)
		return nullptr;
	return new Metadata(metadata);
}


void TypeArchive::RemoveMetadata(const std::string& key) noexcept(false)
{
	if (!BNTypeArchiveRemoveMetadata(m_object, key.c_str()))
		throw DatabaseException("BNTypeArchiveRemoveMetadata");
}
