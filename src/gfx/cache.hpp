#pragma once

#include <string>
#include <unordered_map>

#include "common.hpp"

// TODO: currently, we can't implement RAM caching of objects out of VRAM
// this is because things like Texture2D are too coupled to file paths
// ideally AssetTypes should take a void* or u8* to load from
// TODO: we store two copies of the path/uid, one in the hashmap, the other
// in the Asset, we should avoid this
template<class AssetType>
struct AssetCache {
    struct Asset {
        std::string path;
        usize       ref_count = 0;
        AssetType   asset;
        bool        in_vram     = false;
        bool        in_ram      = false;
        bool        keep_loaded = false;

        Asset(const std::string& path)
        {
            this->path = path;
        }

        Asset(const std::string& uid, const AssetType& asset)
        {
            this->path        = uid;
            this->asset       = asset;
            this->in_vram     = true;
            this->keep_loaded = true;
        }

        void Load_VRAM()
        {
            if (in_vram) {
                return;
            }

            this->asset   = AssetType(this->path);
            this->in_vram = true;
        }

        void Unload_VRAM()
        {
            if (!in_vram) {
                return;
            }

            this->asset.Delete();
            this->in_vram = false;
        }

        AssetType* TakeRef()
        {
            this->Load_VRAM();
            this->ref_count += 1;
            return &this->asset;
        }

        static void ReturnRef(AssetType* ref)
        {
            Asset* wrapper = containerof(ref, Asset, asset);
            ASSERT(wrapper->ref_count > 0);
            wrapper->ref_count -= 1;
        }
    };

    std::unordered_map<std::string, Asset> assets;

    // estimates
    usize ram_bytes_used;
    usize vram_bytes_used;

    AssetCache(usize capacity = 32)
    {
        this->assets.reserve(capacity);
    }

    AssetType* LoadStatic(std::string_view uid, const AssetType&& asset)
    {
        std::string tmp_uid = std::string(uid);
        const auto& elem    = this->assets.find(tmp_uid);

        if (elem == std::end(this->assets)) {
            return this->assets.emplace(tmp_uid, Asset(tmp_uid, asset)).first->second.TakeRef();
        } else {
            return elem->second.TakeRef();
        }
    }

    AssetType* Load(std::string_view path)
    {
        std::string tmp_path = std::string(path);
        const auto& elem     = this->assets.find(tmp_path);

        if (elem == std::end(this->assets)) {
            return this->assets.emplace(tmp_path, Asset(tmp_path)).first->second.TakeRef();
        } else {
            return elem->second.TakeRef();
        }
    }

    void Unload(AssetType* asset)
    {
        Asset::ReturnRef(asset);
    }

    void Compress_VRAM()
    {
        for (const auto& [key, val] : this->assets) {
            val.Unload_VRAM();
        }
    }
};
