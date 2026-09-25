#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <string>
#include <array>
#include <set>
#include <memory>
#include <functional>
#include <algorithm>
#include <cmath>
#include "../preset/PresetManager.h"
#include "../preset/UserPresetBank.h"
#include "../preset/PresetPackager.h"
#include "../plugin/PluginProcessor.h"
#include "NodeComponent.h"

namespace audio_graph {

/**
 * @brief Elemento de lista unificado para el navegador (Presets de Fábrica + Usuario).
 */
struct BrowserPresetItem {
    bool isUserPreset{ false };
    size_t sourceIndex{ 0 };
    std::string name;
    std::string category;
    std::string author;
    std::string description;
    std::vector<std::string> tags;
    bool favorite{ false };
    std::array<float, 8> macros{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    std::string jsonContent;
};

/**
 * @brief Suite de Navegación de Presets Profesional de 3 Columnas inspirada en Arturia y FL Studio FLEX (Reglas 21, 23, 27).
 * - Columna 1 (Izquierda): Colecciones de Bancos, Categorías Temáticas y Chips de Etiquetas/Carácter.
 * - Columna 2 (Centro): Tabla de Presets de alta densidad con búsqueda en vivo, ordenación, filtrado reactivo y audición instantánea.
 * - Columna 3 (Derecha): Tarjeta de Inspección de Preset con Banner Gráfico Generativo, notas del diseñador, medidores de 8 macros y acciones.
 */
class PresetBrowserDrawerComponent : public juce::Component,
                                     public juce::ListBoxModel,
                                     public juce::TextEditor::Listener
{
public:
    explicit PresetBrowserDrawerComponent(N8AudioProcessor& processor)
        : processor_(processor)
    {
        setOpaque(true);
        setWantsKeyboardFocus(true);

        // -------------------------------------------------------------
        // COLUMNA 1: Explorador de Bancos, Categorías y Tags
        // -------------------------------------------------------------
        auto setupBankBtn = [this](juce::TextButton& btn, const juce::String& text, int bankId) {
            btn.setButtonText(text);
            btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff12151e));
            btn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff1a2636));
            btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
            btn.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff00f0ff));
            btn.setClickingTogglesState(true);
            btn.setRadioGroupId(1001);
            btn.onClick = [this, bankId]() {
                selectedBank_ = bankId;
                updateFilter();
            };
            addAndMakeVisible(btn);
        };

        setupBankBtn(bankAllBtn_, juce::String::fromUTF8("◈ ALL PRESETS"), 0);
        setupBankBtn(bankFavBtn_, juce::String::fromUTF8("★ FAVORITES"), 1);
        setupBankBtn(bankFactoryBtn_, juce::String::fromUTF8("⚙ FACTORY BANK"), 2);
        setupBankBtn(bankUserBtn_, juce::String::fromUTF8("👤 USER LIBRARY"), 3);
        bankAllBtn_.setToggleState(true, juce::dontSendNotification);

        // Lista de Categorías
        categoryListBox_.setModel(&categoryModel_);
        categoryListBox_.setRowHeight(26);
        categoryListBox_.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff0a0c12));
        categoryListBox_.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff1c2230));
        addAndMakeVisible(categoryListBox_);

        // Tags temáticos predefinidos estilo FLEX/Arturia
        const std::vector<std::string> standardTags = {
            "Ambient", "Space", "Glitch", "Analog", "Distorted",
            "Cinematic", "Wide", "Lo-Fi", "Warm", "Clean", "Drums", "Vocal"
        };
        for (const auto& tag : standardTags) {
            auto btn = std::make_unique<juce::TextButton>("#" + tag);
            btn->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff12151e));
            btn->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00d4ff));
            btn->setColour(juce::TextButton::textColourOffId, juce::Colour(0xff7c889a));
            btn->setColour(juce::TextButton::textColourOnId, juce::Colours::black);
            btn->setClickingTogglesState(true);
            btn->onClick = [this, tag, ptr = btn.get()]() {
                if (ptr->getToggleState()) {
                    selectedTag_ = tag;
                    // Desactivar otros botones de tag
                    for (auto& other : tagButtons_) {
                        if (other.get() != ptr) other->setToggleState(false, juce::dontSendNotification);
                    }
                } else {
                    selectedTag_.clear();
                }
                updateFilter();
            };
            addAndMakeVisible(*btn);
            tagButtons_.push_back(std::move(btn));
        }

        // -------------------------------------------------------------
        // COLUMNA 2: Tabla Central y Barra de Búsqueda
        // -------------------------------------------------------------
        searchBox_.setTextToShowWhenEmpty("Search presets by name, category, author or tags...", juce::Colour(0xff606c80));
        searchBox_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff0d1017));
        searchBox_.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff222a38));
        searchBox_.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff00f0ff));
        searchBox_.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        searchBox_.addListener(this);
        searchBox_.onTextChange = [this]() { updateFilter(); };
        addAndMakeVisible(searchBox_);

        clearSearchBtn_.setButtonText(juce::String::fromUTF8("✕"));
        clearSearchBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161a24));
        clearSearchBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        clearSearchBtn_.onClick = [this]() {
            searchBox_.clear();
            updateFilter();
        };
        addAndMakeVisible(clearSearchBtn_);

        sortCombo_.addItem("Sort by: Name", 1);
        sortCombo_.addItem("Sort by: Category", 2);
        sortCombo_.addItem("Sort by: Author", 3);
        sortCombo_.addItem("Sort by: Favorites", 4);
        sortCombo_.setSelectedId(1, juce::dontSendNotification);
        sortCombo_.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff0d1017));
        sortCombo_.setColour(juce::ComboBox::textColourId, juce::Colours::white);
        sortCombo_.onChange = [this]() {
            sortMode_ = sortCombo_.getSelectedId();
            applySort();
        };
        addAndMakeVisible(sortCombo_);

        clearFiltersBtn_.setButtonText("Reset Filters");
        clearFiltersBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161a24));
        clearFiltersBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00d4ff));
        clearFiltersBtn_.onClick = [this]() {
            selectedBank_ = 0;
            bankAllBtn_.setToggleState(true, juce::dontSendNotification);
            selectedCategory_ = "All";
            categoryListBox_.selectRow(0);
            selectedTag_.clear();
            for (auto& btn : tagButtons_) btn->setToggleState(false, juce::dontSendNotification);
            searchBox_.clear();
            updateFilter();
        };
        addAndMakeVisible(clearFiltersBtn_);

        // Tabla / ListBox central
        listBox_.setModel(this);
        listBox_.setRowHeight(36);
        listBox_.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff080a10));
        listBox_.setColour(juce::ListBox::outlineColourId, juce::Colour(0xff18202c));
        addAndMakeVisible(listBox_);

        // -------------------------------------------------------------
        // COLUMNA 3: Tarjeta de Inspección de Preset (Arturia / FLEX)
        // -------------------------------------------------------------
        closeBtn_.setButtonText(juce::String::fromUTF8("✕"));
        closeBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a1d28));
        closeBtn_.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        closeBtn_.onClick = [this]() {
            setVisible(false);
            if (onCloseRequested_) onCloseRequested_();
        };
        addAndMakeVisible(closeBtn_);

        prevPresetBtn_.setButtonText(juce::String::fromUTF8("◀ PREV"));
        prevPresetBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff141822));
        prevPresetBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        prevPresetBtn_.onClick = [this]() { stepAudition(-1); };
        addAndMakeVisible(prevPresetBtn_);

        nextPresetBtn_.setButtonText(juce::String::fromUTF8("NEXT ▶"));
        nextPresetBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff141822));
        nextPresetBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8c96a5));
        nextPresetBtn_.onClick = [this]() { stepAudition(1); };
        addAndMakeVisible(nextPresetBtn_);

        loadPresetBtn_.setButtonText("LOAD PRESET");
        loadPresetBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00d4ff));
        loadPresetBtn_.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
        loadPresetBtn_.onClick = [this]() {
            if (selectedFilteredIndex_ >= 0 && selectedFilteredIndex_ < static_cast<int>(filteredIndices_.size())) {
                loadPresetAtIndex(filteredIndices_[static_cast<size_t>(selectedFilteredIndex_)]);
                setVisible(false);
                if (onCloseRequested_) onCloseRequested_();
            }
        };
        addAndMakeVisible(loadPresetBtn_);

        newPresetBtn_.setButtonText("+ SAVE AS NEW");
        newPresetBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161a26));
        newPresetBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00f0ff));
        newPresetBtn_.onClick = [this]() { promptSaveNewPreset(); };
        addAndMakeVisible(newPresetBtn_);

        revealBtn_.setButtonText("REVEAL DIR");
        revealBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161a26));
        revealBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcbd5e1));
        revealBtn_.onClick = [this]() { revealUserPresetDirectory(); };
        addAndMakeVisible(revealBtn_);

        exportPackBtn_.setButtonText("EXPORT .n8pack");
        exportPackBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161a26));
        exportPackBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcbd5e1));
        exportPackBtn_.onClick = [this]() { promptExportPack(); };
        addAndMakeVisible(exportPackBtn_);

        importPackBtn_.setButtonText("IMPORT .n8pack");
        importPackBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff161a26));
        importPackBtn_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00f0ff));
        importPackBtn_.onClick = [this]() { promptImportPack(); };
        addAndMakeVisible(importPackBtn_);

        // Inicializar ruta de presets de usuario en AppData
        auto appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                              .getChildFile("N8Audio")
                              .getChildFile("N8Effect")
                              .getChildFile("UserPresets");
        if (!appDataDir.exists()) {
            appDataDir.createDirectory();
        }
        processor_.getPresetManager().getUserBank().setDirectory(appDataDir.getFullPathName().toStdString());

        reloadPresets();
    }

    void reloadPresets() {
        allPresets_.clear();
        categoriesList_.clear();
        categoriesList_.push_back("All Categories");

        std::set<std::string> catSet;

        // 1. Cargar Factory Presets con metadatos completos y macros
        const auto& factoryList = processor_.getPresetManager().getFactoryPresets();
        for (size_t i = 0; i < factoryList.size(); ++i) {
            BrowserPresetItem item;
            item.isUserPreset = false;
            item.sourceIndex = i;
            item.name = factoryList[i].name;
            item.category = factoryList[i].category;
            item.author = GraphSerializer::extractString(factoryList[i].jsonContent, "author", "N8Audio DSP Lab");
            item.description = GraphSerializer::extractString(factoryList[i].jsonContent, "description", factoryList[i].category);
            item.favorite = (sessionFactoryFavorites_.count(item.name) > 0);
            item.jsonContent = factoryList[i].jsonContent;

            // Extraer macros
            const auto macSec = GraphSerializer::extractArraySection(item.jsonContent, "macros");
            if (!macSec.empty()) {
                auto macs = GraphSerializer::parseNumberList(macSec);
                for (size_t m = 0; m < std::min(macs.size(), size_t{ 8 }); ++m) {
                    item.macros[m] = macs[m];
                }
            }

            // Extraer o inferir tags de estilo
            const auto tagsSec = GraphSerializer::extractArraySection(item.jsonContent, "tags");
            if (!tagsSec.empty()) {
                item.tags = GraphSerializer::parseStringList(tagsSec);
            } else {
                generateSmartTags(item);
            }

            catSet.insert(item.category);
            allPresets_.push_back(std::move(item));
        }

        // 2. Cargar User Presets
        auto& userBank = processor_.getPresetManager().getUserBank();
        userBank.rescan();
        const auto& userList = userBank.getPresets();
        for (size_t i = 0; i < userList.size(); ++i) {
            BrowserPresetItem item;
            item.isUserPreset = true;
            item.sourceIndex = i;
            item.name = userList[i].metadata.name;
            item.category = userList[i].metadata.category.empty() ? "User" : userList[i].metadata.category;
            item.author = userList[i].metadata.author.empty() ? "User" : userList[i].metadata.author;
            item.description = userList[i].metadata.description.empty() ? "User Created Preset" : userList[i].metadata.description;
            item.tags = userList[i].metadata.tags;
            if (item.tags.empty()) item.tags.push_back("User");
            item.favorite = userList[i].metadata.favorite;
            item.jsonContent = userList[i].jsonContent;

            const auto macSec = GraphSerializer::extractArraySection(item.jsonContent, "macros");
            if (!macSec.empty()) {
                auto macs = GraphSerializer::parseNumberList(macSec);
                for (size_t m = 0; m < std::min(macs.size(), size_t{ 8 }); ++m) {
                    item.macros[m] = macs[m];
                }
            }

            catSet.insert(item.category);
            allPresets_.push_back(std::move(item));
        }

        for (const auto& c : catSet) {
            categoriesList_.push_back(c);
        }

        categoryModel_.setCategories(&categoriesList_, [this](int row) {
            if (row >= 0 && row < static_cast<int>(categoriesList_.size())) {
                selectedCategory_ = (row == 0) ? "All" : categoriesList_[static_cast<size_t>(row)];
                updateFilter();
            }
        });
        categoryListBox_.updateContent();

        updateFilter();
    }

    void setOnPresetLoaded(std::function<void()> cb) { onPresetLoaded_ = std::move(cb); }
    void setOnCloseRequested(std::function<void()> cb) { onCloseRequested_ = std::move(cb); }

    juce::String getSelectedPresetName() const {
        if (selectedFilteredIndex_ >= 0 && selectedFilteredIndex_ < static_cast<int>(filteredIndices_.size())) {
            return allPresets_[filteredIndices_[static_cast<size_t>(selectedFilteredIndex_)]].name;
        }
        return {};
    }

    juce::String getSelectedPresetCategory() const {
        if (selectedFilteredIndex_ >= 0 && selectedFilteredIndex_ < static_cast<int>(filteredIndices_.size())) {
            return allPresets_[filteredIndices_[static_cast<size_t>(selectedFilteredIndex_)]].category;
        }
        return {};
    }

    // --- ListBoxModel Implementation (Columna 2) ---
    int getNumRows() override {
        return static_cast<int>(filteredIndices_.size());
    }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override {
        if (rowNumber < 0 || rowNumber >= static_cast<int>(filteredIndices_.size())) return;
        const auto& item = allPresets_[filteredIndices_[static_cast<size_t>(rowNumber)]];
        const juce::Colour catColor = getCategoryColorFromName(item.category);

        // 1. Fondo de la fila con sombreado alternado o resaltado de selección
        if (rowIsSelected) {
            juce::ColourGradient selGrad(catColor.withAlpha(0.25f), 0.0f, 0.0f,
                                         juce::Colour(0x18ffffff), static_cast<float>(width), 0.0f, false);
            g.setGradientFill(selGrad);
            g.fillRect(0, 0, width, height);

            // Borde lateral luminoso cyan/categoría
            g.setColour(catColor);
            g.fillRect(0, 0, 3, height);
            g.drawRect(0, 0, width, height, 1);
        } else {
            g.setColour(rowNumber % 2 == 0 ? juce::Colour(0xff090c12) : juce::Colour(0xff0d1018));
            g.fillRect(0, 0, width, height);
        }

        auto area = juce::Rectangle<float>(6.0f, 2.0f, static_cast<float>(width - 12), static_cast<float>(height - 4));

        // 2. Icono de Favorito ★ / ☆
        auto starArea = area.removeFromLeft(24.0f);
        if (item.favorite) {
            g.setColour(juce::Colour(0xffffcc00));
            g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
            g.drawText(juce::String::fromUTF8("★"), starArea, juce::Justification::centred);
        } else {
            g.setColour(juce::Colour(0xff4a5568));
            g.setFont(juce::FontOptions(13.0f, juce::Font::plain));
            g.drawText(juce::String::fromUTF8("☆"), starArea, juce::Justification::centred);
        }

        // 3. Nombre del Preset (Tipografía bold, blanca nítida)
        auto nameArea = area.removeFromLeft(180.0f);
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.setColour(rowIsSelected ? juce::Colours::white : juce::Colour(0xffe2e8f0));
        g.drawText(item.name, nameArea, juce::Justification::centredLeft, true);

        // 4. Pastilla de Categoría (Pill con borde de color y fondo translúcido)
        auto catArea = area.removeFromLeft(120.0f).reduced(2.0f, 6.0f);
        g.setColour(catColor.withAlpha(0.20f));
        g.fillRoundedRectangle(catArea, 3.0f);
        g.setColour(catColor);
        g.drawRoundedRectangle(catArea, 3.0f, 1.0f);
        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.drawText(juce::String(item.category).toUpperCase(), catArea, juce::Justification::centred, true);

        area.removeFromLeft(10.0f);

        // 5. Tags de Estilo (#Ambient #3D)
        auto tagsArea = area.removeFromLeft(150.0f);
        juce::String tagStr;
        for (size_t t = 0; t < std::min(item.tags.size(), size_t{ 3 }); ++t) {
            tagStr += "#" + juce::String(item.tags[t]) + " ";
        }
        g.setFont(juce::FontOptions(9.5f, juce::Font::plain));
        g.setColour(juce::Colour(0xff718096));
        g.drawText(tagStr, tagsArea, juce::Justification::centredLeft, true);

        // 6. Autor e Insignia de Banco
        g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
        if (item.isUserPreset) {
            g.setColour(juce::Colour(0xff00ffcc));
            g.drawText("[User]", area, juce::Justification::centredRight, true);
        } else {
            g.setColour(juce::Colour(0xffa0aec0));
            g.drawText("[N8 Factory]", area, juce::Justification::centredRight, true);
        }
    }

    void listBoxItemClicked(int row, const juce::MouseEvent& e) override {
        if (row < 0 || row >= static_cast<int>(filteredIndices_.size())) return;
        selectedFilteredIndex_ = row;
        const size_t presetIdx = filteredIndices_[static_cast<size_t>(row)];

        // Clic en la estrella para conmutar favorito
        if (e.position.x < 32.0f) {
            toggleFavoriteAtIndex(presetIdx);
            return;
        }

        // Clic simple: Audición en vivo y actualización de la Tarjeta de Inspección (Columna 3)
        loadPresetAtIndex(presetIdx);
        repaint();
    }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override {
        if (row < 0 || row >= static_cast<int>(filteredIndices_.size())) return;
        loadPresetAtIndex(filteredIndices_[static_cast<size_t>(row)]);
        setVisible(false);
        if (onCloseRequested_) onCloseRequested_();
    }

    bool keyPressed(const juce::KeyPress& key) override {
        if (key == juce::KeyPress::upKey) {
            stepAudition(-1);
            return true;
        }
        if (key == juce::KeyPress::downKey) {
            stepAudition(1);
            return true;
        }
        if (key == juce::KeyPress::returnKey) {
            if (selectedFilteredIndex_ >= 0 && selectedFilteredIndex_ < static_cast<int>(filteredIndices_.size())) {
                loadPresetAtIndex(filteredIndices_[static_cast<size_t>(selectedFilteredIndex_)]);
                setVisible(false);
                if (onCloseRequested_) onCloseRequested_();
            }
            return true;
        }
        if (key == juce::KeyPress::escapeKey) {
            setVisible(false);
            if (onCloseRequested_) onCloseRequested_();
            return true;
        }
        return Component::keyPressed(key);
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // 1. Fondo general de la ventana modal
        g.setColour(juce::Colour(0xff06070a));
        g.fillRoundedRectangle(bounds, 8.0f);

        // Borde exterior metálico
        g.setColour(juce::Colour(0xff1e2532));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.2f);

        auto innerArea = bounds.reduced(10.0f);

        // 2. Columna 1 (Izquierda: Explorador)
        auto col1 = innerArea.removeFromLeft(200.0f);
        g.setColour(juce::Colour(0xff0a0d14));
        g.fillRoundedRectangle(col1, 6.0f);
        g.setColour(juce::Colour(0xff161c28));
        g.drawRoundedRectangle(col1, 6.0f, 1.0f);

        // Etiquetas de sección en Columna 1
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff64748b));
        g.drawText("COLLECTIONS", juce::Rectangle<float>(col1.getX() + 10.0f, col1.getY() + 8.0f, 180.0f, 14.0f), juce::Justification::centredLeft);
        g.drawText("CATEGORIES", juce::Rectangle<float>(col1.getX() + 10.0f, col1.getY() + 144.0f, 180.0f, 14.0f), juce::Justification::centredLeft);
        g.drawText("STYLE & CHARACTER", juce::Rectangle<float>(col1.getX() + 10.0f, col1.getBottom() - 175.0f, 180.0f, 14.0f), juce::Justification::centredLeft);

        innerArea.removeFromLeft(8.0f); // Separador

        // 3. Columna 3 (Derecha: Inspector de Preset)
        auto col3 = innerArea.removeFromRight(320.0f);
        g.setColour(juce::Colour(0xff0a0d14));
        g.fillRoundedRectangle(col3, 6.0f);
        g.setColour(juce::Colour(0xff161c28));
        g.drawRoundedRectangle(col3, 6.0f, 1.0f);

        // Renderizado de la Tarjeta del Inspector en Columna 3
        drawPresetInspectorCard(g, col3);

        innerArea.removeFromRight(8.0f); // Separador

        // 4. Columna 2 (Centro: Tabla)
        auto col2 = innerArea;
        g.setColour(juce::Colour(0xff080a10));
        g.fillRoundedRectangle(col2, 6.0f);
        g.setColour(juce::Colour(0xff161c28));
        g.drawRoundedRectangle(col2, 6.0f, 1.0f);

        // Contador de resultados e indicadores de filtros activos en Columna 2
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff00d4ff));
        juce::String summaryStr = juce::String(filteredIndices_.size()) + " PRESETS";
        if (selectedCategory_ != "All") summaryStr += " // " + juce::String(selectedCategory_).toUpperCase();
        if (!selectedTag_.empty()) summaryStr += " // #" + juce::String(selectedTag_).toUpperCase();
        g.drawText(summaryStr, juce::Rectangle<float>(col2.getX() + 12.0f, col2.getY() + 42.0f, col2.getWidth() - 24.0f, 18.0f), juce::Justification::centredLeft);
    }

    void resized() override {
        auto area = getLocalBounds().reduced(10);

        // -------------------------------------------------------------
        // COLUMNA 1 (Izquierda: 200px)
        // -------------------------------------------------------------
        auto col1 = area.removeFromLeft(200);
        col1.reduce(8, 8);

        col1.removeFromTop(18); // Título COLLECTIONS
        bankAllBtn_.setBounds(col1.removeFromTop(24));
        col1.removeFromTop(3);
        bankFavBtn_.setBounds(col1.removeFromTop(24));
        col1.removeFromTop(3);
        bankFactoryBtn_.setBounds(col1.removeFromTop(24));
        col1.removeFromTop(3);
        bankUserBtn_.setBounds(col1.removeFromTop(24));

        col1.removeFromTop(20); // Título CATEGORIES
        auto tagArea = col1.removeFromBottom(155);

        // Lista de categorías ocupa el espacio central de Columna 1
        categoryListBox_.setBounds(col1);

        // Grid de Tags al pie de Columna 1 (2 columnas de chips)
        tagArea.removeFromTop(16);
        const int tagW = (tagArea.getWidth() - 4) / 2;
        const int tagH = 20;
        int tIdx = 0;
        for (auto& tagBtn : tagButtons_) {
            const int r = tIdx / 2;
            const int c = tIdx % 2;
            tagBtn->setBounds(tagArea.getX() + c * (tagW + 4), tagArea.getY() + r * (tagH + 3), tagW, tagH);
            tIdx++;
        }

        area.removeFromLeft(8); // Separador

        // -------------------------------------------------------------
        // COLUMNA 3 (Derecha: 320px)
        // -------------------------------------------------------------
        auto col3 = area.removeFromRight(320);
        col3.reduce(10, 8);

        // Botón cerrar en la esquina superior derecha
        closeBtn_.setBounds(col3.getX() + col3.getWidth() - 22, col3.getY(), 22, 22);

        // Botones inferiores de Columna 3
        auto utilBar2 = col3.removeFromBottom(22);
        exportPackBtn_.setBounds(utilBar2.removeFromLeft((utilBar2.getWidth() - 4) / 2));
        importPackBtn_.setBounds(utilBar2);

        col3.removeFromBottom(4);
        auto utilBar1 = col3.removeFromBottom(22);
        newPresetBtn_.setBounds(utilBar1.removeFromLeft((utilBar1.getWidth() - 4) / 2));
        revealBtn_.setBounds(utilBar1);

        col3.removeFromBottom(8);
        loadPresetBtn_.setBounds(col3.removeFromBottom(32));

        col3.removeFromBottom(6);
        auto stepBar = col3.removeFromBottom(24);
        prevPresetBtn_.setBounds(stepBar.removeFromLeft((stepBar.getWidth() - 6) / 2));
        stepBar.removeFromLeft(6);
        nextPresetBtn_.setBounds(stepBar);

        // -------------------------------------------------------------
        // COLUMNA 2 (Centro: Tabla)
        // -------------------------------------------------------------
        area.removeFromRight(8); // Separador
        auto col2 = area.reduced(8, 8);

        // Barra superior de Búsqueda y Ordenación
        auto searchRow = col2.removeFromTop(28);
        clearSearchBtn_.setBounds(searchRow.removeFromRight(26));
        searchRow.removeFromRight(4);
        sortCombo_.setBounds(searchRow.removeFromRight(130));
        searchRow.removeFromRight(6);
        clearFiltersBtn_.setBounds(searchRow.removeFromRight(90));
        searchRow.removeFromRight(6);
        searchBox_.setBounds(searchRow);

        col2.removeFromTop(32); // Espacio para resumen de filtros activos

        // Tabla ListBox ocupa el resto
        listBox_.setBounds(col2);
    }

    static juce::Colour getCategoryColorFromName(const std::string& catName) {
        auto c = juce::String(catName).toLowerCase();
        if (c.contains("space") || c.contains("reverb") || c.contains("bloom")) return juce::Colour(0xffa855f7);
        if (c.contains("delay") || c.contains("time") || c.contains("echo")) return juce::Colour(0xff00f0aa);
        if (c.contains("drive") || c.contains("distort") || c.contains("saturat")) return juce::Colour(0xffff3366);
        if (c.contains("dynam") || c.contains("comp") || c.contains("limit") || c.contains("gate")) return juce::Colour(0xffff9900);
        if (c.contains("filter") || c.contains("eq") || c.contains("formant")) return juce::Colour(0xff00d4ff);
        if (c.contains("grain") || c.contains("granular") || c.contains("chaos") || c.contains("glitch")) return juce::Colour(0xffeab308);
        if (c.contains("spectr") || c.contains("fft") || c.contains("freeze")) return juce::Colour(0xffeab308);
        if (c.contains("modulat") || c.contains("phaser") || c.contains("chorus") || c.contains("flanger")) return juce::Colour(0xff38bdf8);
        if (c.contains("spatial") || c.contains("3d") || c.contains("panner")) return juce::Colour(0xff10b981);
        if (c.contains("user")) return juce::Colour(0xff00d4ff);
        return juce::Colour(0xff818cf8);
    }

private:
    // Modelo auxiliar para la lista de categorías de Columna 1
    class CategoryListModel : public juce::ListBoxModel {
    public:
        void setCategories(const std::vector<std::string>* cats, std::function<void(int)> onSelect) {
            categories_ = cats;
            onSelect_ = std::move(onSelect);
        }

        int getNumRows() override {
            return categories_ ? static_cast<int>(categories_->size()) : 0;
        }

        void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool rowIsSelected) override {
            if (!categories_ || row < 0 || row >= static_cast<int>(categories_->size())) return;
            const auto& name = (*categories_)[static_cast<size_t>(row)];

            if (rowIsSelected) {
                g.setColour(juce::Colour(0xff162232));
                g.fillRect(0, 0, w, h);
                g.setColour(juce::Colour(0xff00f0ff));
                g.fillRect(0, 0, 3, h);
                g.setFont(juce::FontOptions(10.5f, juce::Font::bold));
                g.drawText(name, 10, 0, w - 12, h, juce::Justification::centredLeft, true);
            } else {
                g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
                g.setColour(juce::Colour(0xff8c96a5));
                g.drawText(name, 10, 0, w - 12, h, juce::Justification::centredLeft, true);
            }
        }

        void listBoxItemClicked(int row, const juce::MouseEvent&) override {
            if (onSelect_) onSelect_(row);
        }

    private:
        const std::vector<std::string>* categories_{ nullptr };
        std::function<void(int)> onSelect_;
    };

    void drawPresetInspectorCard(juce::Graphics& g, juce::Rectangle<float> area) {
        if (filteredIndices_.empty() || selectedFilteredIndex_ < 0 ||
            selectedFilteredIndex_ >= static_cast<int>(filteredIndices_.size()))
        {
            g.setFont(juce::FontOptions(12.0f));
            g.setColour(juce::Colour(0xff606c80));
            g.drawText("No preset selected", area, juce::Justification::centred);
            return;
        }

        const auto& item = allPresets_[filteredIndices_[static_cast<size_t>(selectedFilteredIndex_)]];
        const juce::Colour catColor = getCategoryColorFromName(item.category);

        auto cardArea = area.reduced(10.0f, 6.0f);

        // 1. Título de cabecera del inspector
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff64748b));
        g.drawText("PRESET INSPECTOR", cardArea.removeFromTop(18.0f), juce::Justification::centredLeft);

        cardArea.removeFromTop(4.0f);

        // 2. Banner Gráfico Generativo de Arte Vectorial (Estilo FLEX / Arturia)
        auto bannerRect = cardArea.removeFromTop(115.0f);
        drawGenerativeArtwork(g, bannerRect, item, catColor);

        cardArea.removeFromTop(10.0f);

        // 3. Título del Preset (16px bold blanco)
        g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        g.setColour(juce::Colours::white);
        g.drawText(item.name, cardArea.removeFromTop(22.0f), juce::Justification::centredLeft, true);

        // 4. Insignia de Autor y Categoría
        auto authorRow = cardArea.removeFromTop(16.0f);
        g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
        g.setColour(juce::Colour(0xff94a3b8));
        g.drawText("Designed by: " + juce::String(item.author), authorRow, juce::Justification::centredLeft, true);

        cardArea.removeFromTop(6.0f);

        // 5. Descripción y Notas de Diseño Sonoro
        g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
        g.setColour(juce::Colour(0xff8c96a5));
        juce::String descStr = item.description.empty() ? "No sound design notes provided." : item.description;
        g.drawFittedText(descStr, cardArea.removeFromTop(42.0f).toNearestInt(), juce::Justification::topLeft, 3);

        cardArea.removeFromTop(6.0f);

        // 6. Indicadores de 8 Performance Macros
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff64748b));
        g.drawText("PERFORMANCE MACROS (8)", cardArea.removeFromTop(16.0f), juce::Justification::centredLeft);

        const char* macroNames[8] = {
            "TEXTURE", "MOTION", "SPACE", "COLOR",
            "CHAOS", "DENSITY", "ENERGY", "MORPH"
        };

        const float macroRowH = 15.0f;
        for (int i = 0; i < 4; ++i) {
            auto row = cardArea.removeFromTop(macroRowH);
            auto leftCol = row.removeFromLeft(row.getWidth() * 0.48f);
            row.removeFromLeft(row.getWidth() * 0.04f);
            auto rightCol = row;

            drawMacroGauge(g, leftCol, macroNames[i * 2], item.macros[static_cast<size_t>(i * 2)], catColor);
            drawMacroGauge(g, rightCol, macroNames[i * 2 + 1], item.macros[static_cast<size_t>(i * 2 + 1)], catColor);
            cardArea.removeFromTop(2.0f);
        }
    }

    void drawMacroGauge(juce::Graphics& g, juce::Rectangle<float> bounds, const char* name, float value, juce::Colour catColor) {
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xffa0aec0));
        g.drawText(name, bounds.removeFromLeft(48.0f), juce::Justification::centredLeft, false);

        g.setColour(juce::Colour(0xff00d4ff));
        juce::String pctStr = juce::String(static_cast<int>(value * 100.0f)) + "%";
        g.drawText(pctStr, bounds.removeFromRight(26.0f), juce::Justification::centredRight, false);

        bounds.removeFromLeft(2.0f);
        bounds.removeFromRight(2.0f);
        auto barRect = bounds.withSizeKeepingCentre(bounds.getWidth(), 4.0f);

        // Pista oscura ranurada
        g.setColour(juce::Colour(0xff06070a));
        g.fillRoundedRectangle(barRect, 2.0f);
        g.setColour(juce::Colour(0xff1a202c));
        g.drawRoundedRectangle(barRect, 2.0f, 0.8f);

        // Barra activa
        const float activeW = barRect.getWidth() * std::clamp(value, 0.0f, 1.0f);
        if (activeW > 1.0f) {
            auto fillRect = barRect.withWidth(activeW);
            juce::ColourGradient grad(catColor, fillRect.getX(), fillRect.getY(),
                                      juce::Colour(0xff00f0ff), fillRect.getRight(), fillRect.getY(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(fillRect, 2.0f);
        }
    }

    void drawGenerativeArtwork(juce::Graphics& g, juce::Rectangle<float> bounds, const BrowserPresetItem& item, juce::Colour catColor) {
        // Fondo base de la tarjeta artística
        g.setColour(juce::Colour(0xff05070a));
        g.fillRoundedRectangle(bounds, 6.0f);

        // Halo radial de luz suave en el color de categoría
        juce::ColourGradient aura(catColor.withAlpha(0.24f), bounds.getCentreX(), bounds.getCentreY(),
                                  juce::Colour(0xff05070a), bounds.getX(), bounds.getY(), true);
        g.setGradientFill(aura);
        g.fillRoundedRectangle(bounds, 6.0f);

        const float w = bounds.getWidth();
        const float h = bounds.getHeight();
        juce::ignoreUnused(h);
        const float cx = bounds.getCentreX();
        const float cy = bounds.getCentreY();
        const auto cName = juce::String(item.category).toLowerCase();

        // Cuadrícula perspectiva de fondo sutil
        g.setColour(catColor.withAlpha(0.08f));
        for (float gx = bounds.getX() + 10.0f; gx < bounds.getRight(); gx += 20.0f) {
            g.drawLine(gx, bounds.getY(), gx, bounds.getBottom(), 0.8f);
        }
        for (float gy = bounds.getY() + 10.0f; gy < bounds.getBottom(); gy += 20.0f) {
            g.drawLine(bounds.getX(), gy, bounds.getRight(), gy, 0.8f);
        }

        // Gráficos vectoriales según categoría temática
        if (cName.contains("space") || cName.contains("reverb") || cName.contains("bloom")) {
            // Reverb / Espacio: Polígono 3D estrellado difuso y constelación de partículas
            juce::Path starPoly;
            const int pts = 8;
            for (int i = 0; i < pts * 2; ++i) {
                float r = (i % 2 == 0) ? 36.0f : 16.0f;
                float angle = static_cast<float>(i) * juce::MathConstants<float>::pi / static_cast<float>(pts);
                float px = cx + std::cos(angle) * r;
                float py = cy + std::sin(angle) * r;
                if (i == 0) starPoly.startNewSubPath(px, py);
                else starPoly.lineTo(px, py);
            }
            starPoly.closeSubPath();
            g.setColour(catColor.withAlpha(0.6f));
            g.strokePath(starPoly, juce::PathStrokeType(1.5f));

            // Partículas de dispersión
            for (int p = 0; p < 12; ++p) {
                float px = cx + std::sin(static_cast<float>(p) * 1.7f) * 48.0f;
                float py = cy + std::cos(static_cast<float>(p) * 2.3f) * 24.0f;
                g.setColour(juce::Colours::white.withAlpha(0.7f));
                g.fillEllipse(px - 1.5f, py - 1.5f, 3.0f, 3.0f);
            }
        }
        else if (cName.contains("delay") || cName.contains("time") || cName.contains("echo")) {
            // Delay / Tiempo: Taps rítmicos y arcos parabólicos de rebote
            const float baseY = cy + 20.0f;
            for (int t = 0; t < 5; ++t) {
                float tapX = bounds.getX() + 30.0f + static_cast<float>(t) * (w - 60.0f) / 4.0f;
                float tapH = 40.0f * std::exp(-static_cast<float>(t) * 0.45f);
                g.setColour(catColor.withAlpha(0.85f - static_cast<float>(t) * 0.15f));
                g.drawLine(tapX, baseY, tapX, baseY - tapH, 2.0f);
                g.fillEllipse(tapX - 3.0f, baseY - tapH - 3.0f, 6.0f, 6.0f);
            }
            // Arco conector
            juce::Path arc;
            arc.startNewSubPath(bounds.getX() + 30.0f, baseY - 40.0f);
            for (int t = 1; t < 5; ++t) {
                float prevX = bounds.getX() + 30.0f + static_cast<float>(t - 1) * (w - 60.0f) / 4.0f;
                float curX = bounds.getX() + 30.0f + static_cast<float>(t) * (w - 60.0f) / 4.0f;
                float curY = baseY - 40.0f * std::exp(-static_cast<float>(t) * 0.45f);
                arc.quadraticTo((prevX + curX) * 0.5f, baseY - 50.0f, curX, curY);
            }
            g.setColour(catColor.withAlpha(0.4f));
            g.strokePath(arc, juce::PathStrokeType(1.0f));
        }
        else if (cName.contains("drive") || cName.contains("distort") || cName.contains("saturat")) {
            // Distorsión: Onda en zigzag saturada de alta energía
            juce::Path driveWave;
            driveWave.startNewSubPath(bounds.getX() + 20.0f, cy);
            for (float x = 20.0f; x < w - 20.0f; x += 12.0f) {
                float rawY = std::sin(x * 0.15f) * 45.0f;
                float clippedY = std::tanh(rawY / 20.0f) * 30.0f;
                driveWave.lineTo(bounds.getX() + x, cy - clippedY);
            }
            g.setColour(catColor.withAlpha(0.85f));
            g.strokePath(driveWave, juce::PathStrokeType(2.0f));

            // Rieles de clipping horizontal
            g.setColour(juce::Colour(0xffff0044).withAlpha(0.4f));
            g.drawHorizontalLine(static_cast<int>(cy - 30.0f), bounds.getX() + 15.0f, bounds.getRight() - 15.0f);
            g.drawHorizontalLine(static_cast<int>(cy + 30.0f), bounds.getX() + 15.0f, bounds.getRight() - 15.0f);
        }
        else if (cName.contains("dynam") || cName.contains("comp") || cName.contains("limit") || cName.contains("gate")) {
            // Dinámica: Codo de compresión logarítmica y pico de transitorio
            juce::Path compKnee;
            compKnee.startNewSubPath(bounds.getX() + 30.0f, bounds.getBottom() - 25.0f);
            compKnee.lineTo(cx, cy);
            compKnee.quadraticTo(cx + 25.0f, cy - 10.0f, bounds.getRight() - 30.0f, cy - 18.0f);
            g.setColour(catColor.withAlpha(0.85f));
            g.strokePath(compKnee, juce::PathStrokeType(2.0f));

            // Indicador de umbral (Threshold)
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.drawVerticalLine(static_cast<int>(cx), cy - 25.0f, cy + 25.0f);
            g.fillEllipse(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
        }
        else if (cName.contains("grain") || cName.contains("granular") || cName.contains("chaos")) {
            // Granular: Nube de micro-granos flotantes con líneas de afinidad
            for (int gIdx = 0; gIdx < 16; ++gIdx) {
                float gx = cx + std::sin(static_cast<float>(gIdx) * 1.9f) * (w * 0.35f);
                float gy = cy + std::cos(static_cast<float>(gIdx) * 2.7f) * 28.0f;
                g.setColour(catColor.withAlpha(0.7f));
                g.fillEllipse(gx - 3.0f, gy - 3.0f, 6.0f, 6.0f);
                if (gIdx > 0) {
                    float px = cx + std::sin(static_cast<float>(gIdx - 1) * 1.9f) * (w * 0.35f);
                    float py = cy + std::cos(static_cast<float>(gIdx - 1) * 2.7f) * 28.0f;
                    g.setColour(catColor.withAlpha(0.2f));
                    g.drawLine(px, py, gx, gy, 0.8f);
                }
            }
        }
        else if (cName.contains("spatial") || cName.contains("3d") || cName.contains("panner")) {
            // Espacial 3D: Elipses orbitales de perspectiva
            g.setColour(catColor.withAlpha(0.6f));
            g.drawEllipse(cx - 50.0f, cy - 25.0f, 100.0f, 50.0f, 1.5f);
            g.drawEllipse(cx - 30.0f, cy - 15.0f, 60.0f, 30.0f, 1.0f);
            g.setColour(juce::Colour(0xff00ffff));
            g.fillEllipse(cx + 35.0f - 4.0f, cy - 12.0f - 4.0f, 8.0f, 8.0f);
            g.fillEllipse(cx - 35.0f - 4.0f, cy + 12.0f - 4.0f, 8.0f, 8.0f);
        }
        else {
            // Categoría General / Modular / Experimental: Figuras de Lissajous y órbitas de fase
            juce::Path lissajous;
            lissajous.startNewSubPath(cx + std::sin(0.0f) * 45.0f, cy + std::sin(0.0f) * 28.0f);
            for (float t = 0.0f; t < juce::MathConstants<float>::twoPi; t += 0.1f) {
                float lx = cx + std::sin(t * 3.0f) * 45.0f;
                float ly = cy + std::sin(t * 2.0f + 0.5f) * 28.0f;
                lissajous.lineTo(lx, ly);
            }
            g.setColour(catColor.withAlpha(0.75f));
            g.strokePath(lissajous, juce::PathStrokeType(1.5f));
        }

        // Pastilla de Categoría en la esquina superior izquierda del banner
        auto pillRect = juce::Rectangle<float>(bounds.getX() + 8.0f, bounds.getY() + 8.0f, 95.0f, 16.0f);
        g.setColour(juce::Colour(0xd00a0d14));
        g.fillRoundedRectangle(pillRect, 3.0f);
        g.setColour(catColor);
        g.drawRoundedRectangle(pillRect, 3.0f, 1.0f);
        g.setFont(juce::FontOptions(8.5f, juce::Font::bold));
        g.drawText(juce::String(item.category).toUpperCase(), pillRect, juce::Justification::centred, true);

        // Borde exterior del banner
        g.setColour(juce::Colour(0xff1e2636));
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
    }

    void updateFilter() {
        filteredIndices_.clear();
        const std::string query = searchBox_.getText().trim().toLowerCase().toStdString();

        for (size_t i = 0; i < allPresets_.size(); ++i) {
            const auto& item = allPresets_[i];

            // 1. Filtro de Colección de Banco
            if (selectedBank_ == 1 && !item.favorite) continue;
            if (selectedBank_ == 2 && item.isUserPreset) continue;
            if (selectedBank_ == 3 && !item.isUserPreset) continue;

            // 2. Filtro de Categoría
            if (selectedCategory_ != "All" && !selectedCategory_.empty()) {
                if (!juce::String(item.category).equalsIgnoreCase(juce::String(selectedCategory_))) continue;
            }

            // 3. Filtro de Tag
            if (!selectedTag_.empty()) {
                bool found = false;
                for (const auto& t : item.tags) {
                    if (juce::String(t).equalsIgnoreCase(juce::String(selectedTag_))) {
                        found = true;
                        break;
                    }
                }
                if (!found) continue;
            }

            // 4. Filtro de Búsqueda instantánea
            if (!query.empty()) {
                const auto nameL = juce::String(item.name).toLowerCase();
                const auto catL = juce::String(item.category).toLowerCase();
                const auto authL = juce::String(item.author).toLowerCase();
                const auto descL = juce::String(item.description).toLowerCase();

                bool tagFound = false;
                for (const auto& t : item.tags) {
                    if (juce::String(t).toLowerCase().contains(query)) {
                        tagFound = true;
                        break;
                    }
                }

                if (!nameL.contains(query) && !catL.contains(query) &&
                    !authL.contains(query) && !descL.contains(query) && !tagFound)
                {
                    continue;
                }
            }

            filteredIndices_.push_back(i);
        }

        applySort();
    }

    void applySort() {
        if (sortMode_ == 1) { // Por Nombre
            std::sort(filteredIndices_.begin(), filteredIndices_.end(), [this](size_t a, size_t b) {
                return juce::String(allPresets_[a].name).toLowerCase() < juce::String(allPresets_[b].name).toLowerCase();
            });
        } else if (sortMode_ == 2) { // Por Categoría
            std::sort(filteredIndices_.begin(), filteredIndices_.end(), [this](size_t a, size_t b) {
                if (allPresets_[a].category == allPresets_[b].category) {
                    return juce::String(allPresets_[a].name).toLowerCase() < juce::String(allPresets_[b].name).toLowerCase();
                }
                return juce::String(allPresets_[a].category).toLowerCase() < juce::String(allPresets_[b].category).toLowerCase();
            });
        } else if (sortMode_ == 3) { // Por Autor
            std::sort(filteredIndices_.begin(), filteredIndices_.end(), [this](size_t a, size_t b) {
                return juce::String(allPresets_[a].author).toLowerCase() < juce::String(allPresets_[b].author).toLowerCase();
            });
        } else if (sortMode_ == 4) { // Por Favorito
            std::sort(filteredIndices_.begin(), filteredIndices_.end(), [this](size_t a, size_t b) {
                if (allPresets_[a].favorite != allPresets_[b].favorite) {
                    return allPresets_[a].favorite > allPresets_[b].favorite;
                }
                return juce::String(allPresets_[a].name).toLowerCase() < juce::String(allPresets_[b].name).toLowerCase();
            });
        }

        if (selectedFilteredIndex_ >= static_cast<int>(filteredIndices_.size())) {
            selectedFilteredIndex_ = filteredIndices_.empty() ? -1 : 0;
        } else if (selectedFilteredIndex_ < 0 && !filteredIndices_.empty()) {
            selectedFilteredIndex_ = 0;
        }

        listBox_.updateContent();
        if (selectedFilteredIndex_ >= 0) {
            listBox_.selectRow(selectedFilteredIndex_);
        }
        repaint();
    }

    void stepAudition(int delta) {
        if (filteredIndices_.empty()) return;
        selectedFilteredIndex_ = std::clamp(selectedFilteredIndex_ + delta, 0, static_cast<int>(filteredIndices_.size()) - 1);
        listBox_.selectRow(selectedFilteredIndex_);
        listBox_.scrollToEnsureRowIsOnscreen(selectedFilteredIndex_);
        loadPresetAtIndex(filteredIndices_[static_cast<size_t>(selectedFilteredIndex_)]);
        repaint();
    }

    void loadPresetAtIndex(size_t index) {
        if (index >= allPresets_.size()) return;
        const auto& item = allPresets_[index];

        if (processor_.loadPresetFromJson(item.jsonContent)) {
            if (onPresetLoaded_) {
                onPresetLoaded_();
            }
        }
    }

    void toggleFavoriteAtIndex(size_t index) {
        if (index >= allPresets_.size()) return;
        auto& item = allPresets_[index];

        if (item.isUserPreset) {
            auto& userBank = processor_.getPresetManager().getUserBank();
            userBank.toggleFavorite(item.sourceIndex);
            reloadPresets();
        } else {
            // Guardar favorito de sesión para presets de fábrica
            if (sessionFactoryFavorites_.count(item.name) > 0) {
                sessionFactoryFavorites_.erase(item.name);
                item.favorite = false;
            } else {
                sessionFactoryFavorites_.insert(item.name);
                item.favorite = true;
            }
            listBox_.updateContent();
            repaint();
        }
    }

    void generateSmartTags(BrowserPresetItem& item) {
        const auto c = juce::String(item.category).toLowerCase();
        if (c.contains("psicoacústica") || c.contains("modulación")) {
            item.tags = { "Modulation", "3D", "Stereo", "Psychoacoustic" };
        } else if (c.contains("espacio") || c.contains("shimmer") || c.contains("reverb")) {
            item.tags = { "Space", "Ambient", "Reverb", "Shimmer" };
        } else if (c.contains("distorsión") || c.contains("agresión") || c.contains("drive")) {
            item.tags = { "Distorted", "Drive", "Aggressive", "Warm" };
        } else if (c.contains("granular") || c.contains("caos") || c.contains("glitch")) {
            item.tags = { "Granular", "Glitch", "Chaos", "Cloud" };
        } else if (c.contains("dinámica") || c.contains("pegada") || c.contains("compres")) {
            item.tags = { "Punch", "Dynamics", "Clean", "Punchy" };
        } else if (c.contains("spectral") || c.contains("freeze")) {
            item.tags = { "Spectral", "Freeze", "Ambient", "FFT" };
        } else if (c.contains("tiempo") || c.contains("retardo") || c.contains("delay")) {
            item.tags = { "Time", "Delay", "Wide", "Echo" };
        } else if (c.contains("lo-fi") || c.contains("vintage") || c.contains("tape")) {
            item.tags = { "Lo-Fi", "Analog", "Warm", "Vintage" };
        } else {
            item.tags = { "Ambient", "Cinematic", "Complex" };
        }
        item.tags.push_back("Factory");
    }

    void promptSaveNewPreset() {
        auto* alert = new juce::AlertWindow("Save New Preset", "Enter metadata for your custom patch:", juce::AlertWindow::QuestionIcon);
        alert->addTextEditor("name", "Custom Nebula", "Preset Name:");
        alert->addTextEditor("category", "Experimental", "Category:");
        alert->addTextEditor("tags", "Ambient, Space, Grain", "Tags (comma separated):");
        alert->addTextEditor("author", "Sound Designer", "Author:");

        alert->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
        alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert](int result) {
            if (result == 1) {
                PresetMetadata meta;
                meta.name = alert->getTextEditorContents("name").trim().toStdString();
                meta.category = alert->getTextEditorContents("category").trim().toStdString();
                meta.author = alert->getTextEditorContents("author").trim().toStdString();
                meta.description = "User created preset in N8Effect Studio";

                auto tagTokens = juce::StringArray::fromTokens(alert->getTextEditorContents("tags"), ",", " ");
                for (const auto& t : tagTokens) {
                    if (!t.isEmpty()) meta.tags.push_back(t.trim().toStdString());
                }

                std::array<float, 8> macros{ 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
                const char* macroIds[8] = {
                    "macro_texture", "macro_motion", "macro_space", "macro_color",
                    "macro_chaos", "macro_density", "macro_energy", "macro_morph"
                };
                for (size_t i = 0; i < 8; ++i) {
                    if (auto* p = processor_.getAPVTS().getParameter(macroIds[i])) {
                        macros[i] = p->getValue();
                    }
                }

                processor_.getPresetManager().getUserBank().savePreset(processor_.getGraph(), meta, macros);
                reloadPresets();
            }
            delete alert;
        }));
    }

    void promptExportPack() {
        fileChooser_ = std::make_unique<juce::FileChooser>(
            "Export N8 Preset Pack (.n8pack)",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.n8pack");

        const auto browserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser_->launchAsync(browserFlags, [this](const juce::FileChooser& fc) {
            const auto file = fc.getResult();
            if (file != juce::File{}) {
                auto& userBank = processor_.getPresetManager().getUserBank();
                PresetPackManifest manifest;
                manifest.packName = file.getFileNameWithoutExtension().toStdString();
                manifest.author = "User";
                manifest.description = "N8Effect Studio Sound Pack";
                manifest.tags = { "Pack", "Community", "Presets" };

                std::string err;
                if (!PresetPackager::exportPack(userBank.getPresets(), manifest, file.getFullPathName().toStdString(), err)) {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Export Error", err);
                } else {
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Export Success", "Preset pack exported successfully!");
                }
            }
        });
    }

    void promptImportPack() {
        fileChooser_ = std::make_unique<juce::FileChooser>(
            "Import N8 Preset Pack (.n8pack)",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.n8pack");

        const auto browserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser_->launchAsync(browserFlags, [this](const juce::FileChooser& fc) {
            const auto file = fc.getResult();
            if (file.existsAsFile()) {
                auto& userBank = processor_.getPresetManager().getUserBank();
                PackImportReport report;
                if (PresetPackager::importPack(file.getFullPathName().toStdString(), userBank.getDirectory(), DuplicatePolicy::RenameWithSuffix, report)) {
                    reloadPresets();
                    juce::String msg = "Successfully imported " + juce::String(report.imported) + " presets!";
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Import Success", msg);
                } else {
                    std::string err = report.errors.empty() ? "Failed to parse pack" : report.errors[0];
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Import Error", err);
                }
            }
        });
    }

    void revealUserPresetDirectory() {
        const auto path = processor_.getPresetManager().getUserBank().getDirectory();
        juce::File(path).revealToUser();
    }

    N8AudioProcessor& processor_;
    std::vector<BrowserPresetItem> allPresets_;
    std::vector<size_t> filteredIndices_;
    std::set<std::string> sessionFactoryFavorites_;

    int selectedBank_{ 0 }; // 0: All, 1: Fav, 2: Factory, 3: User
    std::string selectedCategory_{ "All" };
    std::string selectedTag_;
    int sortMode_{ 1 };
    int selectedFilteredIndex_{ 0 };

    std::vector<std::string> categoriesList_;
    CategoryListModel categoryModel_;

    // Componentes Columna 1
    juce::TextButton bankAllBtn_;
    juce::TextButton bankFavBtn_;
    juce::TextButton bankFactoryBtn_;
    juce::TextButton bankUserBtn_;
    juce::ListBox categoryListBox_;
    std::vector<std::unique_ptr<juce::TextButton>> tagButtons_;

    // Componentes Columna 2
    juce::TextEditor searchBox_;
    juce::TextButton clearSearchBtn_;
    juce::ComboBox sortCombo_;
    juce::TextButton clearFiltersBtn_;
    juce::ListBox listBox_;

    // Componentes Columna 3
    juce::TextButton closeBtn_;
    juce::TextButton prevPresetBtn_;
    juce::TextButton nextPresetBtn_;
    juce::TextButton loadPresetBtn_;
    juce::TextButton newPresetBtn_;
    juce::TextButton revealBtn_;
    juce::TextButton exportPackBtn_;
    juce::TextButton importPackBtn_;

    std::unique_ptr<juce::FileChooser> fileChooser_;
    std::function<void()> onPresetLoaded_;
    std::function<void()> onCloseRequested_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserDrawerComponent)
};

} // namespace audio_graph
