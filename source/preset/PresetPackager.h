#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include "GraphSerializer.h"
#include "UserPresetBank.h"
#include "../dsp/rust_core/include/n8_rust_core.h"

namespace audio_graph {

/**
 * @brief Metadatos descriptivos de un paquete de presets (.n8pack) (Reglas 21, 27).
 */
struct PresetPackManifest {
    uint32_t packVersion{ 1 };
    std::string packName{ "Untitled Pack" };
    std::string author{ "Community" };
    std::string description{ "" };
    std::string version{ "1.0.0" };
    std::string createdAt{ "" };
    std::vector<std::string> tags{};
    std::string format{ "N8Effect_PresetPack" };
    size_t presetCount{ 0 };
};

/**
 * @brief Política de resolución de duplicados al importar paquetes.
 */
enum class DuplicatePolicy : uint8_t {
    Skip = 0,
    Overwrite = 1,
    RenameWithSuffix = 2
};

/**
 * @brief Reporte detallado de la operación de importación de un paquete.
 */
struct PackImportReport {
    size_t totalFound{ 0 };
    size_t imported{ 0 };
    size_t overwritten{ 0 };
    size_t skipped{ 0 };
    std::vector<std::string> importedNames;
    std::vector<std::string> errors;
};

/**
 * @brief Gestor de Empaquetado, Exportación e Importación de Paquetes y Bancos de Presets .n8pack y .n8bank (Reglas 21, 27).
 * Acelerado por Rust n8_rust_core para serialización JSON, validación de manifiestos y desempaquetado de presets.
 */
class PresetPackager {
public:
    static constexpr uint32_t CurrentPackVersion = 1;

    static std::string serializePack(const std::vector<UserPresetRecord>& presets,
                                     const PresetPackManifest& manifest,
                                     const std::string& format = "N8Effect_PresetPack")
    {
        std::string tagsCsv;
        for (size_t i = 0; i < manifest.tags.size(); ++i) {
            tagsCsv += manifest.tags[i];
            if (i + 1 < manifest.tags.size()) tagsCsv += ",";
        }

        RustPackBuilder* builder = n8_rust_pack_builder_create(
            format.c_str(),
            manifest.packName.c_str(),
            manifest.author.c_str(),
            manifest.description.c_str(),
            manifest.version.c_str(),
            manifest.createdAt.c_str(),
            tagsCsv.c_str()
        );

        if (!builder) return "{}";

        for (const auto& rec : presets) {
            n8_rust_pack_builder_add_preset(
                builder,
                rec.metadata.name.c_str(),
                rec.metadata.category.c_str(),
                rec.metadata.author.c_str(),
                rec.jsonContent.c_str()
            );
        }

        std::vector<char> buf(presets.size() * 2048 + 4096, 0);
        size_t required = n8_rust_pack_builder_serialize(builder, buf.data(), buf.size());
        if (required >= buf.size()) {
            buf.resize(required + 1, 0);
            n8_rust_pack_builder_serialize(builder, buf.data(), buf.size());
        }

        n8_rust_pack_builder_destroy(builder);
        return std::string(buf.data());
    }

    static std::string serializeBank(const std::vector<UserPresetRecord>& presets,
                                     const PresetPackManifest& manifest)
    {
        return serializePack(presets, manifest, "N8Effect_Bank");
    }

    static bool exportPack(const std::vector<UserPresetRecord>& presets,
                           PresetPackManifest manifest,
                           const std::string& destinationFilePath,
                           std::string& outError)
    {
        if (presets.empty()) {
            outError = "Cannot export empty preset pack";
            return false;
        }

        manifest.presetCount = presets.size();
        if (manifest.createdAt.empty()) {
            manifest.createdAt = "2026-09-24";
        }

        std::string json = serializePack(presets, manifest, "N8Effect_PresetPack");

        std::error_code ec;
        auto parentPath = std::filesystem::path(destinationFilePath).parent_path();
        if (!parentPath.empty() && !std::filesystem::exists(parentPath, ec)) {
            std::filesystem::create_directories(parentPath, ec);
        }

        std::ofstream outFile(destinationFilePath, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!outFile.is_open()) {
            outError = "Failed to open destination file for writing: " + destinationFilePath;
            return false;
        }

        outFile.write(json.data(), static_cast<std::streamsize>(json.size()));
        outFile.close();
        return true;
    }

    static bool exportBank(const std::vector<UserPresetRecord>& presets,
                           PresetPackManifest manifest,
                           const std::string& destinationFilePath,
                           std::string& outError)
    {
        if (presets.empty()) {
            outError = "Cannot export empty preset bank";
            return false;
        }

        manifest.presetCount = presets.size();
        if (manifest.createdAt.empty()) {
            manifest.createdAt = "2026-09-24";
        }

        std::string json = serializeBank(presets, manifest);

        std::error_code ec;
        auto parentPath = std::filesystem::path(destinationFilePath).parent_path();
        if (!parentPath.empty() && !std::filesystem::exists(parentPath, ec)) {
            std::filesystem::create_directories(parentPath, ec);
        }

        std::ofstream outFile(destinationFilePath, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!outFile.is_open()) {
            outError = "Failed to open destination bank file for writing: " + destinationFilePath;
            return false;
        }

        outFile.write(json.data(), static_cast<std::streamsize>(json.size()));
        outFile.close();
        return true;
    }

    static bool inspectPackManifest(std::string_view packContent,
                                    PresetPackManifest& outManifest,
                                    std::string& outError)
    {
        std::string contentStr(packContent);
        char formatBuf[64] = { 0 };
        uint32_t packVer = 1;
        char nameBuf[256] = { 0 };
        char authorBuf[256] = { 0 };
        char descBuf[1024] = { 0 };
        char verBuf[64] = { 0 };
        char createdBuf[64] = { 0 };
        size_t count = 0;
        char tagsBuf[1024] = { 0 };
        char errBuf[512] = { 0 };

        bool ok = n8_rust_preset_packager_inspect_manifest(
            contentStr.c_str(),
            formatBuf, sizeof(formatBuf),
            &packVer,
            nameBuf, sizeof(nameBuf),
            authorBuf, sizeof(authorBuf),
            descBuf, sizeof(descBuf),
            verBuf, sizeof(verBuf),
            createdBuf, sizeof(createdBuf),
            &count,
            tagsBuf, sizeof(tagsBuf),
            errBuf, sizeof(errBuf)
        );

        if (!ok) {
            outError = errBuf;
            return false;
        }

        outManifest.format = formatBuf;
        outManifest.packVersion = packVer;
        outManifest.packName = nameBuf;
        outManifest.author = authorBuf;
        outManifest.description = descBuf;
        outManifest.version = verBuf;
        outManifest.createdAt = createdBuf;
        outManifest.presetCount = count;

        outManifest.tags.clear();
        std::stringstream ss(tagsBuf);
        std::string tag;
        while (std::getline(ss, tag, ',')) {
            if (!tag.empty()) outManifest.tags.push_back(tag);
        }

        return true;
    }

    static bool importPackContent(std::string_view packContent,
                                  const std::string& targetDirectory,
                                  DuplicatePolicy policy,
                                  PackImportReport& report)
    {
        PresetPackManifest manifest;
        std::string manifestErr;
        if (!inspectPackManifest(packContent, manifest, manifestErr)) {
            report.errors.push_back(manifestErr);
            return false;
        }

        std::error_code ec;
        if (!std::filesystem::exists(targetDirectory, ec)) {
            std::filesystem::create_directories(targetDirectory, ec);
        }

        std::string contentStr(packContent);
        RustPackExtractor* extractor = n8_rust_pack_extractor_create(contentStr.c_str());
        if (!extractor) {
            report.errors.push_back("Failed to create Rust pack extractor");
            return false;
        }

        size_t total = n8_rust_pack_extractor_get_count(extractor);
        report.totalFound = total;

        if (total == 0) {
            report.errors.push_back("No presets array found in pack");
            n8_rust_pack_extractor_destroy(extractor);
            return false;
        }

        char nameBuf[256];
        char catBuf[256];
        char authorBuf[256];

        for (size_t i = 0; i < total; ++i) {
            size_t contentLen = 0;
            if (!n8_rust_pack_extractor_get_preset_info(
                extractor, i,
                nameBuf, sizeof(nameBuf),
                catBuf, sizeof(catBuf),
                authorBuf, sizeof(authorBuf),
                &contentLen
            )) {
                report.errors.push_back("Failed to query preset index: " + std::to_string(i));
                continue;
            }

            std::string content;
            content.resize(contentLen + 1, 0);
            if (!n8_rust_pack_extractor_get_preset_content(extractor, i, content.data(), content.size())) {
                report.errors.push_back(std::string("Failed to extract content for: ") + nameBuf);
                continue;
            }
            content.resize(contentLen);

            if (content.empty()) {
                report.errors.push_back(std::string("Empty content for preset: ") + nameBuf);
                continue;
            }

            // Validar deserialización del preset
            Graph dummyGraph;
            PresetMetadata dummyMeta;
            std::array<float, 8> dummyMacros{};
            std::string parseErr;
            if (!GraphSerializer::deserialize(content, dummyGraph, dummyMeta, dummyMacros, parseErr)) {
                report.errors.push_back(std::string("Preset parsing failed for '") + nameBuf + "': " + parseErr);
                continue;
            }

            std::string safeName = sanitizeFilename(nameBuf);
            auto targetPath = std::filesystem::path(targetDirectory) / (safeName + ".n8preset");

            if (std::filesystem::exists(targetPath, ec)) {
                if (policy == DuplicatePolicy::Skip) {
                    report.skipped++;
                    continue;
                } else if (policy == DuplicatePolicy::RenameWithSuffix) {
                    int counter = 1;
                    while (std::filesystem::exists(targetPath, ec)) {
                        targetPath = std::filesystem::path(targetDirectory) / (safeName + " (" + std::to_string(counter++) + ").n8preset");
                    }
                    report.imported++;
                } else {
                    // Overwrite
                    report.overwritten++;
                }
            } else {
                report.imported++;
            }

            std::ofstream outFile(targetPath, std::ios::out | std::ios::trunc | std::ios::binary);
            if (outFile.is_open()) {
                outFile.write(content.data(), static_cast<std::streamsize>(content.size()));
                outFile.close();
                report.importedNames.push_back(nameBuf);
            } else {
                report.errors.push_back("Failed to write preset to disk: " + targetPath.string());
            }
        }

        n8_rust_pack_extractor_destroy(extractor);
        return report.imported > 0 || report.overwritten > 0;
    }

    static bool importPack(const std::string& packFilePath,
                           const std::string& targetDirectory,
                           DuplicatePolicy policy,
                           PackImportReport& report)
    {
        std::ifstream inFile(packFilePath, std::ios::in | std::ios::binary);
        if (!inFile.is_open()) {
            report.errors.push_back("Failed to open pack file: " + packFilePath);
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(inFile)),
                            std::istreambuf_iterator<char>());
        inFile.close();

        return importPackContent(content, targetDirectory, policy, report);
    }

    static bool importBank(const std::string& bankFilePath,
                           const std::string& targetDirectory,
                           DuplicatePolicy policy,
                           PackImportReport& report)
    {
        return importPack(bankFilePath, targetDirectory, policy, report);
    }

private:
    static std::string sanitizeFilename(std::string_view name) {
        std::string res;
        for (char c : name) {
            if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                res += '_';
            } else {
                res += c;
            }
        }
        if (res.empty()) res = "Untitled";
        return res;
    }
};

} // namespace audio_graph
