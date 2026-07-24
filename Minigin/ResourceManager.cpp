#include <stdexcept>
#include "ResourceManager.h"
#include "Renderer.h"
#include "Texture2D.h"

namespace fs = std::filesystem;

void portfolio::ResourceManager::Init(const std::filesystem::path& dataPath)
{
	m_dataPath = dataPath;
}

std::shared_ptr<portfolio::Texture2D> portfolio::ResourceManager::LoadTexture(const std::string& file)
{
	const auto fullPath = m_dataPath/file;
	const auto filename = fs::path(fullPath).filename().string();
	if(m_loadedTextures.find(filename) == m_loadedTextures.end())
		m_loadedTextures.insert(std::pair(filename,std::make_shared<Texture2D>(fullPath.string())));
	return m_loadedTextures.at(filename);
}

void portfolio::ResourceManager::UnloadUnusedResources()
{
	for (auto it = m_loadedTextures.begin(); it != m_loadedTextures.end();)
	{
		if (it->second.use_count() == 1)
			it = m_loadedTextures.erase(it);
		else
			++it;
	}
}
