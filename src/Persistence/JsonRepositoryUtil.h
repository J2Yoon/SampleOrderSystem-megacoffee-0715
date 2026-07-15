#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "../Json/JsonIO.h"
#include "../Json/JsonValue.h"

namespace Persistence
{
    // Json{Entity}Repository 3종(Sample/Order/ProductionQueue)이 공통으로 필요로 하는
    // "벡터 로드/파일 재기록/키로 원소 찾기" 로직을 한 곳에 모은 템플릿 유틸리티(Template Method 방식).
    // 엔티티별 직렬화(ToJson/FromJson)와 키 추출자는 호출부에서 함수 객체로 주입한다.
    // 파일이 없거나 파싱에 실패하면 예외를 던지지 않고 빈 목록으로 폴백하는 정책은 그대로 유지한다.
    namespace JsonRepositoryUtil
    {
        template <typename TEntity, typename TFromJson>
        std::vector<TEntity> LoadEntitiesFromFile(const std::string& filePath, TFromJson fromJson)
        {
            std::vector<TEntity> entities;

            const auto fileContents = Json::FileIO::ReadAllText(filePath);
            if (!fileContents.has_value())
            {
                return entities; // 파일이 없으면 빈 목록으로 시작한다(최초 실행 등).
            }

            Json::Value root;
            if (!Json::Value::Parse(*fileContents, root) || !root.IsArray())
            {
                return entities; // 파싱 실패 시에도 예외 없이 빈 목록으로 폴백한다.
            }

            for (const auto& item : root.Items())
            {
                entities.push_back(fromJson(item));
            }
            return entities;
        }

        template <typename TEntity, typename TToJson>
        void PersistEntitiesToFile(const std::string& filePath, const std::vector<TEntity>& entities, TToJson toJson)
        {
            Json::Value root = Json::Value::MakeArray();
            for (const auto& entity : entities)
            {
                root.Add(toJson(entity));
            }
            Json::FileIO::WriteAllText(filePath, root.Dump());
        }

        template <typename TEntity, typename TKeyExtractor>
        typename std::vector<TEntity>::iterator FindIteratorByKey(
            std::vector<TEntity>& entities, const std::string& key, TKeyExtractor keyExtractor)
        {
            return std::find_if(entities.begin(), entities.end(),
                [&key, &keyExtractor](const TEntity& entity) { return keyExtractor(entity) == key; });
        }

        template <typename TEntity, typename TKeyExtractor>
        typename std::vector<TEntity>::const_iterator FindIteratorByKey(
            const std::vector<TEntity>& entities, const std::string& key, TKeyExtractor keyExtractor)
        {
            return std::find_if(entities.begin(), entities.end(),
                [&key, &keyExtractor](const TEntity& entity) { return keyExtractor(entity) == key; });
        }
    }
}
