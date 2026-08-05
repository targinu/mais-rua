#include "Achievements.h"

namespace Achievements
{
    namespace
    {
        // Strings sem acento de proposito: literais acentuados ja causaram
        // mojibake com o encoding do compilador (ver comentario sobre o "TM"
        // em PluginEditor.cpp), e o resto da UI ("Classico", "Grao") segue a
        // mesma convencao.
        constexpr Info kList[] = {
            { Id::semFreio,            "sem_freio",            "Sem freio",            "Levou o Rua a 100%." },
            { Id::modoNoturno,         "modo_noturno",         "Modo Noturno",         "A rua virou noite." },
            { Id::voltouProChao,       "voltou_pro_chao",      "Voltou pro chao",      "Foi de uma ponta a outra do knob em menos de 1 segundo." },
            { Id::vizinhancaTranquila, "vizinhanca_tranquila", "Vizinhanca tranquila", "Ficou 3 minutos sem tocar no knob." },
            { Id::rodouQuarteirao,     "rodou_quarteirao",     "Rodou o quarteirao",   "Girou o knob o suficiente pra dar varias voltas no bairro." },
            { Id::cidadeFantasma,      "cidade_fantasma",      "Cidade fantasma",      "Fez todos os predios sumirem." },
            { Id::pichacao,            "pichacao",             "Pichacao",             "Deixou a casinha pichada por tempo suficiente." },
            { Id::buracoNaPista,       "buraco_na_pista",      "Buraco na pista",      "Descobriu os 3 buracos da rua." },
            { Id::graoFino,            "grao_fino",            "Grao fino",            "Trocou pro modo Grao." },
            { Id::indeciso,            "indeciso",             "Indeciso",             "Trocou de modo varias vezes rapidinho." },
            { Id::bemVindoRua,         "bem_vindo_rua",        "Bem-vindo a rua",      "Abriu o Mais Rua pela primeira vez." },
            { Id::sessaoLonga,         "sessao_longa",         "Sessao longa",         "Deixou o plugin aberto por 10 minutos seguidos." },
        };

        static_assert (juce::numElementsInArray (kList) == (size_t) Id::count,
                        "todo Id precisa de uma entrada correspondente em kList");
    }

    const Info& getInfo (Id id)
    {
        return kList[(size_t) id];
    }

    Manager::Manager()
    {
        load();
    }

    juce::File Manager::getFile()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                    .getChildFile ("FrozenShade")
                    .getChildFile ("MaisRua")
                    .getChildFile ("achievements.json");
    }

    void Manager::load()
    {
        auto file = getFile();
        if (! file.existsAsFile())
            return;

        auto data = juce::JSON::parse (file);
        if (auto* arr = data.getArray())
        {
            for (auto& v : *arr)
            {
                const auto keyStr = v.toString();
                for (auto& info : kList)
                    if (keyStr == info.key)
                        unlocked[(size_t) info.id] = true;
            }
        }
    }

    void Manager::save() const
    {
        juce::Array<juce::var> keys;
        for (auto& info : kList)
            if (unlocked[(size_t) info.id])
                keys.add (juce::String (info.key));

        auto file = getFile();
        file.getParentDirectory().createDirectory();
        file.replaceWithText (juce::JSON::toString (juce::var (keys)));
    }

    void Manager::unlock (Id id)
    {
        if (unlocked[(size_t) id])
            return;

        unlocked[(size_t) id] = true;
        pendingNotifications.push_back (id);
        save();
    }

    const Info* Manager::popNextNotification()
    {
        if (pendingNotifications.empty())
            return nullptr;

        const auto id = pendingNotifications.front();
        pendingNotifications.erase (pendingNotifications.begin());
        return &getInfo (id);
    }
}
