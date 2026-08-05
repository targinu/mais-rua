#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <vector>

// Sistema de conquistas: definicoes + persistencia global (fora do estado do
// plugin, que e por projeto). Uma vez desbloqueada, uma conquista fica
// desbloqueada pra sempre em qualquer projeto/instancia, num arquivo no
// AppData do usuario.
namespace Achievements
{
    enum class Id
    {
        semFreio,
        modoNoturno,
        voltouProChao,
        vizinhancaTranquila,
        rodouQuarteirao,
        cidadeFantasma,
        pichacao,
        buracoNaPista,
        graoFino,
        indeciso,
        bemVindoRua,
        sessaoLonga,
        count
    };

    struct Info
    {
        Id id;
        const char* key;    // usado na persistencia, nao muda entre versoes
        const char* title;
        const char* description;
    };

    const Info& getInfo (Id id);

    class Manager
    {
    public:
        Manager();

        bool isUnlocked (Id id) const noexcept { return unlocked[(size_t) id]; }
        void unlock (Id id);

        // Consome e retorna a proxima conquista recem-desbloqueada ainda nao
        // mostrada na UI, ou nullptr se nao ha nenhuma pendente.
        const Info* popNextNotification();

    private:
        static juce::File getFile();
        void load();
        void save() const;

        std::array<bool, (size_t) Id::count> unlocked {};
        std::vector<Id> pendingNotifications;
    };
}
