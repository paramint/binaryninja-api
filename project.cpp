// Copyright (c) 2015-2023 Vector 35 Inc
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
#include <cstring>
#include <optional>
#include "binaryninjaapi.h"
#include "binaryninjacore.h"

using namespace BinaryNinja;


bool ProjectNotification::BeforeOpenProjectCallback(void* ctxt, BNProject* object)
{
	ProjectNotification* notify = (ProjectNotification*)ctxt;
	Ref<Project> project = new Project(BNNewProjectReference(object));
	return notify->OnBeforeOpenProject(project);
}


void ProjectNotification::AfterOpenProjectCallback(void* ctxt, BNProject* object)
{
	ProjectNotification* notify = (ProjectNotification*)ctxt;
	Ref<Project> project = new Project(BNNewProjectReference(object));
	notify->OnAfterOpenProject(project);
}


bool ProjectNotification::BeforeCloseProjectCallback(void* ctxt, BNProject* object)
{
	ProjectNotification* notify = (ProjectNotification*)ctxt;
	Ref<Project> project = new Project(BNNewProjectReference(object));
	return notify->OnBeforeCloseProject(project);
}


void ProjectNotification::AfterCloseProjectCallback(void* ctxt, BNProject* object)
{
	ProjectNotification* notify = (ProjectNotification*)ctxt;
	Ref<Project> project = new Project(BNNewProjectReference(object));
	notify->OnAfterCloseProject(project);
}


void ProjectNotification::ProjectMetadataWrittenCallback(void* ctxt, BNProject* object, char* key, char* value)
{
	ProjectNotification* notify = (ProjectNotification*)ctxt;
	Ref<Project> project = new Project(BNNewProjectReference(object));
	std::string keyStr = key;
	BNFreeString(key);
	std::string valueStr = value;
	BNFreeString(value);
	notify->OnProjectMetadataWritten(project, keyStr, valueStr);
}


void ProjectNotification::ProjectFileCreatedCallback(void* ctxt, BNProject* object, BNProjectFile* bnProjectFile)
{
	ProjectNotification* notify = (ProjectNotification*)ctxt;
	Ref<Project> project = new Project(BNNewProjectReference(object));
	Ref<ProjectFile> projectFile = new ProjectFile(BNNewProjectFileReference(bnProjectFile));
	notify->OnProjectFileCreated(project, projectFile);
}


void ProjectNotification::ProjectFileUpdatedCallback(void* ctxt, BNProject* object, BNProjectFile* bnProjectFile)
{
	ProjectNotification* notify = (ProjectNotification*)ctxt;
	Ref<Project> project = new Project(BNNewProjectReference(object));
	Ref<ProjectFile> projectFile = new ProjectFile(BNNewProjectFileReference(bnProjectFile));
	notify->OnProjectFileUpdated(project, projectFile);
}


void ProjectNotification::ProjectFileDeletedCallback(void* ctxt, BNProject* object, BNProjectFile* bnProjectFile)
{
	ProjectNotification* notify = (ProjectNotification*)ctxt;
	Ref<Project> project = new Project(BNNewProjectReference(object));
	Ref<ProjectFile> projectFile = new ProjectFile(BNNewProjectFileReference(bnProjectFile));
	notify->OnProjectFileDeleted(project, projectFile);
}


void ProjectNotification::ProjectFolderCreatedCallback(void* ctxt, BNProject* object, BNProjectFolder* bnProjectFolder)
{
	ProjectNotification* notify = (ProjectNotification*)ctxt;
	Ref<Project> project = new Project(BNNewProjectReference(object));
	Ref<ProjectFolder> projectFolder = new ProjectFolder(BNNewProjectFolderReference(bnProjectFolder));
	notify->OnProjectFolderCreated(project, projectFolder);
}


void ProjectNotification::ProjectFolderUpdatedCallback(void* ctxt, BNProject* object, BNProjectFolder* bnProjectFolder)
{
	ProjectNotification* notify = (ProjectNotification*)ctxt;
	Ref<Project> project = new Project(BNNewProjectReference(object));
	Ref<ProjectFolder> projectFolder = new ProjectFolder(BNNewProjectFolderReference(bnProjectFolder));
	notify->OnProjectFolderUpdated(project, projectFolder);
}


void ProjectNotification::ProjectFolderDeletedCallback(void* ctxt, BNProject* object, BNProjectFolder* bnProjectFolder)
{
	ProjectNotification* notify = (ProjectNotification*)ctxt;
	Ref<Project> project = new Project(BNNewProjectReference(object));
	Ref<ProjectFolder> projectFolder = new ProjectFolder(BNNewProjectFolderReference(bnProjectFolder));
	notify->OnProjectFolderDeleted(project, projectFolder);
}


ProjectNotification::ProjectNotification()
{
	m_callbacks.context = this;
	m_callbacks.beforeOpenProject = BeforeOpenProjectCallback;
	m_callbacks.afterOpenProject = AfterOpenProjectCallback;
	m_callbacks.beforeCloseProject = BeforeCloseProjectCallback;
	m_callbacks.afterCloseProject = AfterCloseProjectCallback;
	m_callbacks.projectMetadataWritten = ProjectMetadataWrittenCallback;
	m_callbacks.projectFileCreated = ProjectFileCreatedCallback;
	m_callbacks.projectFileUpdated = ProjectFileUpdatedCallback;
	m_callbacks.projectFileDeleted = ProjectFileDeletedCallback;
	m_callbacks.projectFolderCreated = ProjectFolderCreatedCallback;
	m_callbacks.projectFolderUpdated = ProjectFolderUpdatedCallback;
	m_callbacks.projectFolderDeleted = ProjectFolderDeletedCallback;
}


Project::Project(BNProject* project)
{
	m_object = project;
}


Ref<Project> Project::CreateProject(const std::string& path, const std::string& name)
{
	BNProject* bnproj = BNCreateProject(path.c_str(), name.c_str());
	if (!bnproj)
		return nullptr;
	return new Project(bnproj);
}


Ref<Project> Project::OpenProject(const std::string& path)
{
	BNProject* bnproj = BNOpenProject(path.c_str());
	if (!bnproj)
		return nullptr;
	return new Project(bnproj);

}


bool Project::Open()
{
	return BNProjectOpen(m_object);
}


bool Project::Close()
{
	return BNProjectClose(m_object);
}


std::string Project::GetId() const
{
	char* id = BNProjectGetId(m_object);
	std::string result = id;
	BNFreeString(id);
	return result;
}


bool Project::IsOpen() const
{
	return BNProjectIsOpen(m_object);
}


std::string Project::GetPath() const
{
	char* path = BNProjectGetPath(m_object);
	std::string result = path;
	BNFreeString(path);
	return result;
}


std::string Project::GetName() const
{
	char* name = BNProjectGetName(m_object);
	std::string result = name;
	BNFreeString(name);
	return result;
}


void Project::SetName(const std::string& name)
{
	BNProjectSetName(m_object, name.c_str());
}


std::string Project::GetDescription() const
{
	char* description = BNProjectGetDescription(m_object);
	std::string result = description;
	BNFreeString(description);
	return result;
}


void Project::SetDescription(const std::string& description)
{
	BNProjectSetDescription(m_object, description.c_str());
}


std::optional<std::string> Project::ReadMetadata(const std::string& key)
{
	char* value = BNProjectReadMetadata(m_object, key.c_str());
	if (value == nullptr)
		return {};
	std::string result = value;
	BNFreeString(value);
	return result;
}


void Project::WriteMetadata(const std::string& key, const std::string& value)
{
	BNProjectWriteMetadata(m_object, key.c_str(), value.c_str());
}


void Project::DeleteMetadata(const std::string& key)
{
	BNProjectDeleteMetadata(m_object, key.c_str());
}


bool Project::PathExists(Ref<ProjectFolder> folder, const std::string& name) const
{
	LogWarn("Path exists called");
	return false;
}


Ref<ProjectFolder> Project::CreateFolderFromPath(const std::string& path, Ref<ProjectFolder> parent, const std::string& description)
{
	BNProjectFolder* folder = BNProjectCreateFolderFromPath(m_object, path.c_str(), parent ? parent->m_object : nullptr, description.c_str());
	if (folder == nullptr)
		return nullptr;
	return new ProjectFolder(folder);
}


Ref<ProjectFolder> Project::CreateFolder(Ref<ProjectFolder> parent, const std::string& name, const std::string& description)
{
	BNProjectFolder* folder = BNProjectCreateFolder(m_object, parent ? parent->m_object : nullptr, name.c_str(), description.c_str());
	if (folder == nullptr)
		return nullptr;
	return new ProjectFolder(folder);
}


std::vector<Ref<ProjectFolder>> Project::GetFolders() const
{
	size_t count;
	BNProjectFolder** folders = BNProjectGetFolders(m_object, &count);

	std::vector<Ref<ProjectFolder>> result;
	result.reserve(count);
	for (size_t i = 0; i < count; i++)
	{
		result.push_back(new ProjectFolder(BNNewProjectFolderReference(folders[i])));
	}

	BNFreeProjectFolderList(folders, count);
	return result;
}


Ref<ProjectFolder> Project::GetFolderById(const std::string& id) const
{
	BNProjectFolder* folder = BNProjectGetFolderById(m_object, id.c_str());
	if (folder == nullptr)
		return nullptr;
	return new ProjectFolder(folder);
}


void Project::PushFolder(Ref<ProjectFolder> folder)
{
	BNProjectPushFolder(m_object, folder->m_object);
}


void Project::DeleteFolder(Ref<ProjectFolder> folder)
{
	BNProjectDeleteFolder(m_object, folder->m_object);
}


Ref<ProjectFile> Project::CreateFileFromPath(const std::string& path, Ref<ProjectFolder> folder, const std::string& name, const std::string& description)
{
	BNProjectFile* file = BNProjectCreateFileFromPath(m_object, path.c_str(), folder ? folder->m_object : nullptr, name.c_str(), description.c_str());
	if (file == nullptr)
		return nullptr;
	return new ProjectFile(file);
}


Ref<ProjectFile> Project::CreateFile(const std::vector<uint8_t>& contents, Ref<ProjectFolder> folder, const std::string& name, const std::string& description)
{
	BNProjectFile* file = BNProjectCreateFile(m_object, contents.data(), contents.size(), folder ? folder->m_object : nullptr, name.c_str(), description.c_str());
	if (file == nullptr)
		return nullptr;
	return new ProjectFile(file);
}


std::vector<Ref<ProjectFile>> Project::GetFiles() const
{
	size_t count;
	BNProjectFile** files = BNProjectGetFiles(m_object, &count);

	std::vector<Ref<ProjectFile>> result;
	result.reserve(count);
	for (size_t i = 0; i < count; i++)
	{
		result.push_back(new ProjectFile(BNNewProjectFileReference(files[i])));
	}

	BNFreeProjectFileList(files, count);
	return result;
}


Ref<ProjectFile> Project::GetFileById(const std::string& id) const
{
	BNProjectFile* file = BNProjectGetFileById(m_object, id.c_str());
	if (file == nullptr)
		return nullptr;
	return new ProjectFile(file);
}


void Project::PushFile(Ref<ProjectFile> file)
{
	BNProjectPushFile(m_object, file->m_object);
}


void Project::DeleteFile(Ref<ProjectFile> file)
{
	BNProjectDeleteFile(m_object, file->m_object);
}


void Project::RegisterNotification(ProjectNotification* notify)
{
	BNRegisterProjectNotification(m_object, notify->GetCallbacks());
}


void Project::UnregisterNotification(ProjectNotification* notify)
{
	BNUnregisterProjectNotification(m_object, notify->GetCallbacks());
}


ProjectFile::ProjectFile(BNProjectFile* file)
{
	m_object = file;
}


Ref<Project> ProjectFile::GetProject() const
{
	return new Project(BNProjectFileGetProject(m_object));
}


std::string ProjectFile::GetPathOnDisk() const
{
	char* path = BNProjectFileGetPathOnDisk(m_object);
	std::string result = path;
	BNFreeString(path);
	return result;
}


bool ProjectFile::ExistsOnDisk() const
{
	return BNProjectFileExistsOnDisk(m_object);
}


std::string ProjectFile::GetName() const
{
	char* name = BNProjectFileGetName(m_object);
	std::string result = name;
	BNFreeString(name);
	return result;
}


std::string ProjectFile::GetDescription() const
{
	char* description = BNProjectFileGetDescription(m_object);
	std::string result = description;
	BNFreeString(description);
	return result;
}


void ProjectFile::SetName(const std::string& name)
{
	BNProjectFileSetName(m_object, name.c_str());
}


void ProjectFile::SetDescription(const std::string& description)
{
	BNProjectFileSetDescription(m_object, description.c_str());
}


std::string ProjectFile::GetId() const
{
	char* id = BNProjectFileGetId(m_object);
	std::string result = id;
	BNFreeString(id);
	return result;
}


Ref<ProjectFolder> ProjectFile::GetFolder() const
{
	BNProjectFolder* folder = BNProjectFileGetFolder(m_object);
	if (!folder)
		return nullptr;
	return new ProjectFolder(folder);
}


void ProjectFile::SetFolder(Ref<ProjectFolder> folder)
{
	BNProjectFileSetFolder(m_object, folder ? folder->m_object : nullptr);
}


ProjectFolder::ProjectFolder(BNProjectFolder* folder)
{
	m_object = folder;
}


Ref<Project> ProjectFolder::GetProject() const
{
	return new Project(BNProjectFolderGetProject(m_object));
}


std::string ProjectFolder::GetId() const
{
	char* id = BNProjectFolderGetId(m_object);
	std::string result = id;
	BNFreeString(id);
	return result;
}


std::string ProjectFolder::GetName() const
{
	char* name = BNProjectFolderGetName(m_object);
	std::string result = name;
	BNFreeString(name);
	return result;
}


std::string ProjectFolder::GetDescription() const
{
	char* desc = BNProjectFolderGetDescription(m_object);
	std::string result = desc;
	BNFreeString(desc);
	return result;
}


void ProjectFolder::SetName(const std::string& name)
{
	BNProjectFolderSetName(m_object, name.c_str());
}


void ProjectFolder::SetDescription(const std::string& description)
{
	BNProjectFolderSetDescription(m_object, description.c_str());
}


Ref<ProjectFolder> ProjectFolder::GetParent() const
{
	BNProjectFolder* parent = BNProjectFolderGetParent(m_object);
	if (!parent)
		return nullptr;
	return new ProjectFolder(parent);
}


void ProjectFolder::SetParent(Ref<ProjectFolder> parent)
{
	BNProjectFolderSetParent(m_object, parent ? parent->m_object : nullptr);
}
