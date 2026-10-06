// Lang.h -- traductions. Un fichier lang/<code>.bin par langue (produit par tools/convert_assets.py
// depuis world/lang/<code>.txt). Cle absente de la langue choisie : repli sur la langue par defaut
// (fr), puis sur la cle elle-meme (visible a l'ecran = traduction a ajouter).
// Ajouter une langue = ajouter un fichier world/lang/<code>.txt : aucun code a modifier.
#pragma once
#include <string>
#include <unordered_map>
#include "IAssetSource.h"

class Lang {
  public:
    static constexpr const char* kDefault = "fr";
    // Charge la langue par defaut puis `code` par-dessus. Un code inconnu = langue par defaut.
    bool load( IAssetSource& src, const char* code );
    const char* code() const { return current.c_str(); }
    const char* get( const std::string& key ) const;

  private:
    std::string current = kDefault;
    std::unordered_map<std::string, std::string> base, over;
    static bool readFile( IAssetSource& src, const char* code, std::unordered_map<std::string, std::string>& out );
};
