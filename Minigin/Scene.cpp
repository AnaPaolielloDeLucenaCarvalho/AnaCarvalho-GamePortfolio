#include <algorithm>
#include "Scene.h"

using namespace portfolio;

void Scene::Add(std::unique_ptr<GameObject> object)
{
	assert(object != nullptr && "Cannot add a null GameObject to the scene.");
	m_objects.emplace_back(std::move(object));
}

void Scene::Remove(const GameObject& object)
{
	for (auto& obj : m_objects)
	{
		if (obj.get() == &object)
		{
			obj->MarkForDestroy();
			break;
		}
	}
}

void Scene::RemoveAll()
{
	m_objects.clear();
}

void Scene::Update(float deltaTime)
{
	for(auto& object : m_objects)
	{
		object->Update(deltaTime);
	}

	std::erase_if(m_objects, [](const std::unique_ptr<GameObject>& obj) { return obj->IsMarkedForDestroy(); });
}

void Scene::Render() const
{
	std::vector<GameObject*> sortedObjects;
	sortedObjects.reserve(m_objects.size());
	for (const auto& obj : m_objects)
	{
		sortedObjects.push_back(obj.get());
	}

	std::stable_sort(sortedObjects.begin(), sortedObjects.end(), [](GameObject* a, GameObject* b) 
	{
		if (a->GetLayer() != b->GetLayer())
		{
			return a->GetLayer() < b->GetLayer();
		}
		return a->GetTransform().GetPosition().y < b->GetTransform().GetPosition().y;
	});

	for (const auto& object : sortedObjects)
	{
		object->Render();
	}
}

