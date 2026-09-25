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
 * @brief Gestor de Empaquetado, Exportación e Importación de Paquetes de Presets .n8pack (Reglas 21, 27).
 * Empaqueta colecciones completas de presets con metadatos y manifiesto en un único archivo
 * autocontenido de intercambio.
 */
class PresetPackager {
public:
    static constexpr uint32_t CurrentPackVersion = 1;

    static std::string serializePack(const std::vector<UserPresetRecord>& presets,
                                     const PresetPackManifest& manifest)
    {
        std::ostringstream ss;
        ss << "{\n";
        ss << "  \"format\": \"N8Effect_PresetPack\",\n";
        ss << "  \"packVersion\": " << CurrentPackVersion << ",\n";
        ss << "  \"manifest\": {\n";
        ss << "    \"packName\": \"" << GraphSerializer::escapeJson(manifest.packName) << "\",\n";
        ss << "    \"author\": \"" << GraphSerializer::escapeJson(manifest.author) << "\",\n";
        ss << "    \"description\": \"" << GraphSerializer::escapeJson(manifest.description) << "\",\n";
        ss << "    \"version\": \"" << GraphSerializer::escapeJson(manifest.version) << "\",\n";
        ss << "    \"createdAt\": \"" << GraphSerializer::escapeJson(manifest.createdAt) << "\",\n";
        ss << "    \"presetCount\": " << presets.size() << ",\n";
        ss << "    \"tags\": [";
        for (size_t i = 0; i < manifest.tags.size(); ++i) {
            ss << "\"" << GraphSerializer::escapeJson(manifest.tags[i]) << "\"" << (i + 1 < manifest.tags.size() ? ", " : "");
        }
        ss << "]\n";
        ss << "  },\n";

        ss << "  \"presets\": [\n";
        for (size_t i = 0; i < presets.size(); ++i) {
            const auto& rec = presets[i];
            ss << "    {\n";
            ss << "      \"name\": \"" << GraphSerializer::escapeJson(rec.metadata.name) << "\",\n";
            ss << "      \"category\": \"" << GraphSerializer::escapeJson(rec.metadata.category) << "\",\n";
            ss << "      \"author\": \"" << GraphSerializer::escapeJson(rec.metadata.author) << "\",\n";
            ss << "      \"content\": \"" << GraphSerializer::escapeJson(rec.jsonContent) << "\"\n";
            ss << "    }" << (i + 1 < presets.size() ? ",\n" : "\n");
        }
        ss << "  ]\n";
        ss << "}\n";

        return ss.str();
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

        std::string json = serializePack(presets, manifest);

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

    static bool inspectPackManifest(std::string_view packContent,
                                    PresetPackManifest& outManifest,
                                    std::string& outError)
    {
        std::string format = GraphSerializer::extractString(packContent, "format", "");
        if (format != "N8Effect_PresetPack") {
            outError = "Invalid pack format: missing or unrecognized 'format' header";
            return false;
        }

        auto manifestObj = GraphSerializer::extractObjectSection(packContent, "manifest");
        if (manifestObj.empty()) {
            outError = "Corrupted pack: missing 'manifest' section";
            return false;
        }

        outManifest.packVersion = GraphSerializer::extractUInt(packContent, "packVersion", 1);
        outManifest.packName = GraphSerializer::extractString(manifestObj, "packName", "Untitled Pack");
        outManifest.author = GraphSerializer::extractString(manifestObj, "author", "Unknown");
        outManifest.description = GraphSerializer::extractString(manifestObj, "description", "");
        outManifest.version = GraphSerializer::extractString(manifestObj, "version", "1.0.0");
        outManifest.createdAt = GraphSerializer::extractString(manifestObj, "createdAt", "");
        outManifest.presetCount = GraphSerializer::extractUInt(manifestObj, "presetCount", 0);

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

        auto presetsArray = GraphSerializer::extractArraySection(packContent, "presets");
        if (presetsArray.empty()) {
            report.errors.push_back("No presets array found in pack");
            return false;
        }

        auto presetObjects = GraphSerializer::splitObjects(presetsArray);
        report.totalFound = presetObjects.size();

        for (const auto& obj : presetObjects) {
            std::string name = GraphSerializer::extractString(obj, "name", "Imported Preset");
            std::string content = unescapeJson(GraphSerializer::extractString(obj, "content", ""));

            if (content.empty()) {
                report.errors.push_back("Empty content for preset: " + name);
                continue;
            }

            // Validar deserialización del preset
            Graph dummyGraph;
            PresetMetadata dummyMeta;
            std::array<float, 8> dummyMacros{};
            std::string parseErr;
            if (!GraphSerializer::deserialize(content, dummyGraph, dummyMeta, dummyMacros, parseErr)) {
                report.errors.push_back("Preset parsing failed for '" + name + "': " + parseErr);
                continue;
            }

            std::string safeName = sanitizeFilename(name);
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
                report.importedNames.push_back(name);
            } else {
                report.errors.push_back("Failed to write preset to disk: " + targetPath.string());
            }
        }

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

    static std::string unescapeJson(std::string_view str) {
        std::string res;
        res.reserve(str.size());
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '\\' && i + 1 < str.size()) {
                char next = str[i + 1];
                if (next == '"') { res += '"'; i++; }
                else if (next == '\\') { res += '\\'; i++; }
                else if (next == 'n') { res += '\n'; i++; }
                else if (next == 'r') { res += '\r'; i++; }
                else if (next == 't') { res += '\t'; i++; }
                else { res += str[i]; }
            } else {
                res += str[i];
            }
        }
        return res;
    }
};

} // namespace audio_graph
