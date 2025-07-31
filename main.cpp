#include <iostream>
#include <windows.h>
#include <conio.h>
#include <vector>
#include <fstream>
#include <time.h>
#include <chrono>
#include <algorithm>
#include <iomanip> // para setw e setfill
#include <mmsystem.h>





using namespace std;

const int MAP_ALTURA = 25;
const int MAP_LARGURA = 34;

int mapaAtual[25][34];

enum TipoItem {
    VIDA_EXTRA,
    AUMENTO_VELOCIDADE,
    TIROS_EXTRA,
    TIROS_MULTIPLOS,
    PONTOS_EXTRA,
    CONGELAR_INIMIGOS
};

struct jogador
{
    int x, y;
    int vida = 3;
    int pontuacao;
    bool vivo = true;
    int tipoNave = 1;
} jogador1, jogador2;

struct inimigos
{
    int x, y;
    int vida;
};

struct projetil
{
    int x, y;
    int tipo = 0;
};

struct barreiras
{
    int x, y;
    int vida = 3;
};

struct ItemDropado {
    int x, y;
    TipoItem tipo;
    chrono::steady_clock::time_point tempoCriacao;
};

struct EfeitosAtivos {
    bool tirosExtra = false;
    chrono::steady_clock::time_point inicioTirosExtra;

    bool tiroMultiplo = false;
    chrono::steady_clock::time_point inicioTiroMultiplo;


    bool velocidade = false;
    chrono::steady_clock::time_point inicioVelocidade;

    // você pode adicionar outros efeitos aqui
};

EfeitosAtivos efeitosJogador1, efeitosJogador2;

vector<ItemDropado> itensDropados;
vector<barreiras> barreirasDeProtecao;
vector<inimigos> inimigosbasicos;
vector<inimigos> inimigosIntermediarios;
vector<inimigos> inimigosFortes;
vector<inimigos> Boss;
vector<projetil> projeteis;
vector<projetil> projeteisInimigos;







// Variaveis globais de tempo
std::chrono::steady_clock::time_point tempoExplosao;

auto ultimoDesgasteBarreiras = chrono::steady_clock::now();
auto ultimoUpdateItensDropados = chrono::steady_clock::now();


chrono::steady_clock::time_point ultimoDanoJogador1;
chrono::steady_clock::time_point ultimoDanoJogador2;
chrono::steady_clock::time_point ultimoMovimentoJogador2;
chrono::steady_clock::time_point ultimoMovimentoJogador1;

chrono::steady_clock::time_point ultimoMovimentoItem = chrono::steady_clock::now();

// Variaveis globais de jogador/inimigo
string jogadorVenceu;

int direcaoInimigos = 1; // 1 = direita, -1 = esquerda
int delayMovimentoJogador1 = 1;
int contadorMovimentoJ1 = 0;
int delayMovimentoJogador2 = 1;
int contadorMovimentoJ2 = 0;
int escolhaDaNave1 = 1;
int escolhaDaNave2 = 1;
int tipoDeNave1 = 1;
int tipoDeNave2 = 1;

bool precisaDescer = false;
bool projetilInimigosExiste = 0;
bool jogador1Piscando = false;
bool jogador2Piscando = false;

bool coletado = false;

bool jogador1IA = 0; // ativa/desativa o bot

bool inimigosCongelados = false;
chrono::steady_clock::time_point inicioCongelamento;



// Variaveis globais de projetil
time_t ultimoTiroInimigo = 0;
const int intervaloTiro = 2; // segundos entre os tiros inimigos

bool projeteisPiscando = false;
bool explosaoAtiva = false;

int explosaoX = -1;
int explosaoY = -1;

// variaveis globais geral do game
string mensagemCombate = " "; //string para eventuais mensagens que possam aparecer
string dataAtual;

int dificuldade = 1;
int nivel = 1;

bool modoMultiplayer = false;
bool modoInfinito = 0;




bool carregarMapa(const string& caminho, int destino[MAP_ALTURA][MAP_LARGURA])
{
    ifstream arquivo(caminho);
    if (!arquivo.is_open())
    {
        cout << "Erro ao abrir o arquivo: " << caminho << endl;
        return false;
    }

    string linha;
    int i = 0;

    while (getline(arquivo, linha) && i < MAP_ALTURA)
    {
        for (int j = 0; j < MAP_LARGURA && j < linha.size(); j++)
        {
            destino[i][j] = linha[j] - '0'; // converte '1' -> 1
        }
        i++;
    }

    arquivo.close();
    return true;
}

bool ehBoss(int i, int j)
{
    for (auto &inimigos : Boss)
    {
        if (inimigos.x == i && inimigos.y == j) return true;
    }
    return false;
}

bool ehInimigo(int i, int j)
{
    for (auto &inimigos : inimigosbasicos)
    {
        if (inimigos.x == i && inimigos.y == j) return true;
    }
    return false;
}

bool ehInimigoIntermediario(int i, int j)
{
    for (auto &inimigos : inimigosIntermediarios)
    {
        if (inimigos.x == i && inimigos.y == j) return true;
    }
    return false;
}

bool ehInimigoForte(int i, int j)
{
    for (auto &inimigos : inimigosFortes)
    {
        if (inimigos.x == i && inimigos.y == j) return true;
    }
    return false;
}

bool ehBarreira(int i, int j)
{
    for (auto &barreiras : barreirasDeProtecao)
    {
        if (barreiras.x == i && barreiras.y == j) return true;
    }
    return false;
}

bool ehitemdrop(int i, int j)
{
    for (auto &ItemDropado: itensDropados)
    {
        if (ItemDropado.x == i && ItemDropado.y == j) return true;
    }
    return false;
}

bool existeProjetilTipo(int tipo)
{
    for (auto& p : projeteis)
    {
        if (p.tipo == tipo)
            return true;
    }
    return false;
}


void atualizarData()
{
    time_t agora = time(nullptr);
    tm* dataLocal = localtime(&agora);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y", dataLocal); // Formato DD/MM/AAAA
    dataAtual = string(buffer);
}

void salvarPontuacao(string nome, int pontos, int tempoTotal)
{

    ofstream arquivo("Ranking.txt", ios::app);
    if (arquivo.is_open())
    {
        arquivo << "Nome: " << nome << " | Pontos: " << pontos
                << " | Tempo: " << tempoTotal << "s | Data: " << dataAtual << " | Venceu? " << jogadorVenceu << endl;
        arquivo.close();
    }
}

void mostrarRanking()
{
    struct RegistroRanking
    {
        string nome;
        int pontos;
        int tempo;
        string data;
        string venceu;
    };

    vector<RegistroRanking> registros;
    ifstream arquivo("Ranking.txt");

    if (arquivo.is_open())
    {
        string linha;
        while (getline(arquivo, linha))
        {
            RegistroRanking r;
            size_t pos;

            // Nome
            pos = linha.find("Nome: ");
            if (pos != string::npos)
            {
                size_t fim = linha.find(" |", pos);
                r.nome = linha.substr(pos + 6, fim - (pos + 6));
            }

            // Pontos
            pos = linha.find("Pontos: ");
            if (pos != string::npos)
            {
                size_t fim = linha.find(" |", pos);
                r.pontos = stoi(linha.substr(pos + 8, fim - (pos + 8)));
            }

            // Tempo
            pos = linha.find("Tempo: ");
            if (pos != string::npos)
            {
                size_t fim = linha.find("s |", pos);
                r.tempo = stoi(linha.substr(pos + 7, fim - (pos + 7)));
            }

            // Data
            pos = linha.find("Data: ");
            if (pos != string::npos)
            {
                size_t fim = linha.find(" |", pos);
                r.data = linha.substr(pos + 6, fim - (pos + 6));
            }

            // Venceu
            pos = linha.find("Venceu? ");
            if (pos != string::npos)
            {
                r.venceu = linha.substr(pos + 8);
            }

            registros.push_back(r);
        }
        arquivo.close();
    }

    // Ordenar por pontuação (desc) e nome (asc)
    sort(registros.begin(), registros.end(), [](const RegistroRanking &a, const RegistroRanking &b)
    {
        if (a.pontos != b.pontos)
            return a.pontos > b.pontos;
        return a.nome < b.nome;
    });

    // Exibir
    system("cls");
    cout << "\n\n\033[93m===== RANKING DOS JOGADORES =====\033[0m\n";
    for (int i = 0; i < registros.size() && i < 5; i++)
    {
        string cor = (i == 0) ? "\033[92m" : (i == 1) ? "\033[94m" : "\033[97m";
        cout << cor << i + 1 << ". " << registros[i].nome
             << " | Pontos: " << registros[i].pontos
             << " | Tempo: " << registros[i].tempo << "s"
             << " | Data: " << registros[i].data
             << " | Venceu? " << registros[i].venceu
             << "\033[0m" << endl;
    }
    cout << "\033[93m=================================\033[0m\n";

    cout << "\nPressione qualquer tecla para avancar...\n";
    _getch();
}


void escolherNave()
{
    int opcao = -1;

    do
    {
        system("cls");
        cout << "===== ESCOLHA A NAVE DO J1 =====\n";
        cout << "1 - Nave Agil (Tiro simples, movimentacao rapida)\n";
        cout << "2 - Nave de Tiro Extra (Dois tiros, media movimentacao)\n";
        cout << "3 - Nave de Tiro Multiplo (Tiro espalhado, movimentacao lenta)\n";
        cout << "0 - Voltar ao menu anterior\n";
        cout << "Escolha (1, 2 ou 3): ";
        cin >> opcao;

        if (opcao >= 1 && opcao <= 3)
        {
            tipoDeNave1 = opcao;
            switch (opcao)
            {
            case 1:
                delayMovimentoJogador1 = 100;
                break;
            case 2:
                delayMovimentoJogador1 = 200;
                break;
            case 3:
                delayMovimentoJogador1 = 200;
                break;
            }
            escolhaDaNave1 = opcao;
        }

    }
    while (opcao != 0 && (opcao < 1 || opcao > 3));

    if (modoMultiplayer)
    {
        int opcao2 = -1;
        do
        {
            system("cls");
            cout << "===== ESCOLHA A NAVE DO J2 =====\n";
            cout << "1 - Nave Agil (Tiro simples, movimentacao rapida)\n";
            cout << "2 - Nave de Tiro Extra (Dois tiros, média movimentacao)\n";
            cout << "3 - Nave de Tiro Multiplo (Tiro espalhado, movimentacao lenta)\n";
            cout << "0 - Voltar ao menu anterior\n";
            cout << "Escolha (1, 2 ou 3): ";
            cin >> opcao2;

            if (opcao2 >= 1 && opcao2 <= 3)
            {
                tipoDeNave2 = opcao2;
                switch (opcao2)
                {
                case 1:
                    delayMovimentoJogador2 = 100;
                    break;
                case 2:
                    delayMovimentoJogador2 = 200;
                    break;
                case 3:
                    delayMovimentoJogador2 = 200;
                    break;
                }
                escolhaDaNave2 = opcao2;
            }

        }
        while (opcao2 != 0 && (opcao2 < 1 || opcao2 > 3));
    }
}




void mostrarAutores()
{
    system("cls");
    cout << "\033[95m";
    cout << R"(
 ____  _     _____  ____  ____  _____ ____
/  _ \/ \ /\/__ __\/  _ \/  __\/  __// ___\
| / \|| | ||  / \  | / \||  \/||  \  |    \
| |-||| \_/|  | |  | \_/||    /|  /_ \___ |
\_/ \|\____/  \_/  \____/\_/\_\\____\\____/

)";
    cout << "\033[0m\n";

    cout << "\n\033[96m=========== AUTORES ===========\033[0m\n";
    cout << "-> Lucas \"Luqueta\n";
    cout << "-> Joao\n";
    cout << "===============================\n";
    cout << "\nPressione qualquer tecla para voltar...\n";

    while (!_kbhit()) {}
    system("cls");
}

void regrasDicas() {
    system("cls");
    cout << "\033[95m========= REGRAS E CONTROLES =========\033[0m\n";
    cout << "\033[94mControles:\033[0m\n";
    cout << "  A / D     -> Mover jogador 1\n";
    cout << "  <- / ->     -> Mover jogador 2\n";
    cout << "  Espaco    -> Atirar (Jogador 1)\n";
    cout << "  / (barra) -> Atirar (Jogador 2)\n";
    cout << "\n\033[93mObjetivo:\033[0m\n";
    cout << "  Derrote todos os inimigos e sobreviva ao ataque!\n";
    cout << "\n\033[92mBoa sorte!\033[0m\n";
    cout << "\nPressione qualquer tecla para voltar...\n";

    while (!_kbhit()) {}
    system("cls");
}


void escolhaDifuculdade() {

    int opcao = -1;

    do
    {
        system("cls");
        cout << "===== ESCOLHA A DIFICULDADE =====\n";
        cout << "1 - Facil (Inimigos tem 1 de vida)\n";
        cout << "2 - Medio (Inimigos intermediarios e forte tem 2 de vida)\n";
        cout << "3 - Dificil (Inimigos intermediarios e forte tem 3 de vida)\n";
        cout << "0 - Voltar ao menu anterior\n";
        cout << "Escolha (1, 2 ou 3): ";
        cin >> opcao;

        if (opcao >= 1 && opcao <= 3)
        {
            tipoDeNave1 = opcao;
            switch (opcao)
            {
            case 1:
                dificuldade = 1;
                break;
            case 2:
                dificuldade = 2;
                break;
            case 3:
                dificuldade = 3;
                break;
            }
        }
    }
    while (opcao != 0 && (opcao < 1 || opcao > 3));

}


void mostrarMenuInicio()
{
    system("cls");
    cout << "\033[95m";
    cout << R"(
 _  _     _     ____  ____  ____  ____  _________    _________  ____  ____  ____  _  ____  _  ____
/ \/ \  // \ |\/  _ \/ ___\/  _ \/  __\/  __/ ___\  /  __/ ___\/  __\/  _ \/   _\/ \/  _ \/ \/ ___\
| || |\ || | //| / \||    \| / \||  \/||  \ |    \  |  \ |    \|  \/|| / \||  /  | || / \|| ||    \
| || | \|| \// | |-||\___ || \_/||    /|  /_\___ |  |  /_\___ ||  __/| |-|||  \_ | || |-||| |\___ |
\_/\_/  \\__/  \_/ \|\____/\____/\_/\_\\____\____/  \____\____/\_/   \_/ \|\____/\_/\_/ \|\_/\____/

)";
    cout << "\033[0m";

    cout << "\n\033[94m========= MENU PRINCIPAL =========\033[0m\n";
    cout << "1. Ver regras e controles\n";
    cout << "2. Ver ranking de jogadores\n";
    cout << "3. Consultar autores\n";
    cout << "4. Jogar\n";
    cout << "5. Alternar Modo Infinito: " << (modoInfinito ? "\033[92mAtivado\033[0m" : "\033[91mDesativado\033[0m") << "\n";
    cout << "6. Alternar Modo Multiplayer: " << (modoMultiplayer ? "\033[92mAtivado\033[0m" : "\033[91mDesativado\033[0m") << "\n";
    cout << "7. Escolher nave: Nave jogador 1: " << "\033[32m" << escolhaDaNave1 << "\x1b[0m";
    if (modoMultiplayer) {
        cout << " | Nave jogador 2: " << "\033[36m" << escolhaDaNave2 << "\x1b[0m";
        }
    cout << "\n8. Escolher Dificuldade: Dificuldade Atual: " << "\033[32m" << dificuldade << "\x1b[0m";
    cout << "\n9. Bot joga sozinho? " << (jogador1IA ? "\033[92mAtivado\033[0m" : "\033[91mDesativado\033[0m");
    cout << "\n0. Sair\n";
    cout << "\033[94m==================================\033[0m\n";
    cout << "Escolha uma opcao: ";
}


int menuInicio()
{
    int opcao = -1;
    while (opcao != 4 && opcao != 0)
    {
        mostrarMenuInicio();
        cin >> opcao;
        switch(opcao)
        {
        case 1:
            regrasDicas();
            break;
        case 2:
            mostrarRanking();
            break;
        case 3:
            mostrarAutores();
            break;
        case 4:
            break; // vai iniciar o jogo
        case 5:
            modoInfinito = !modoInfinito;
            break;
        case 6:
            modoMultiplayer = !modoMultiplayer;
            jogador1IA = 0;
            break;
        case 7:
            escolherNave();
            break;
        case 8:
            escolhaDifuculdade();
            break;
        case 9:
            jogador1IA = !jogador1IA;
            modoMultiplayer = 0;
            break;
        case 0:
            exit(0);
        default:
            cout << "Opcao invalida. Pressione qualquer tecla para continuar...";
            _getch();
            break;
        }
    }
    return opcao;
}


void desenharHUDLateral(time_t tempoInicial)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos;
    int baseX = MAP_LARGURA + 2;
    int y = 0;

    int tempoDecorrido = difftime(time(NULL), tempoInicial);
    int horas = tempoDecorrido / 3600;
    int minutos = (tempoDecorrido % 3600) / 60;
    int segundos = tempoDecorrido % 60;

    auto mover = [&](int linha) {
        pos.X = baseX;
        pos.Y = linha;
        SetConsoleCursorPosition(hConsole, pos);
    };

    string dificuldadeStr = (dificuldade == 1) ? "Facil" : (dificuldade == 2) ? "Medio" : "Dificil";

    mover(y++); cout << "+--------------------------+";
    mover(y++); cout << "|   STATUS DO JOGO         |";
    mover(y++); cout << "+--------------------------+";

    mover(y++); cout << "| Nivel Atual: " << setw(12) << left << nivel << "|";
    mover(y++); cout << "| Pontos: " << setw(17) << left << jogador1.pontuacao << "|";
    mover(y++); cout << "| Vida J1: " << setw(16) << left << jogador1.vida << "|";

    if (modoMultiplayer) {
        mover(y++); cout << "| Vida J2: " << setw(17) << left << jogador2.vida << "|";
    }

    char buffer[20];
    sprintf(buffer, "%02d:%02d:%02d", horas, minutos, segundos);
    mover(y++); cout << "| Tempo: " << setw(18) << left << buffer << "|";

    mover(y++); cout << "| Dificuldade: " << setw(12) << left << dificuldadeStr << "|";
    mover(y++); cout << "| Multiplayer: " << (modoMultiplayer ? "Ativado   " : "Desativado") << "  |";

    mover(y++); cout << "+--------------------------+";
    mover(y++); cout << "| Ultimo evento:           |";

    string evento = mensagemCombate.substr(0, 23);
    while (evento.length() < 23) evento += " ";
    mover(y++); cout << "| " << evento << "  |";
    mover(y++); cout << "+--------------------------+";
}


void inicializarIniimigosN1()
{
    for (int i = 0; i < 9; i++)
    {
        inimigosbasicos.push_back({4, 3 + i * 2, 1}); // sempre 1 de vida
    }

    int vidaIntermediario = (dificuldade == 1) ? 1 : (dificuldade == 2) ? 2 : 3;
    int vidaForte = (dificuldade == 1) ? 1 : (dificuldade == 2) ? 2 : 3;

    for (int i = 0; i < 9; i++)
    {
        inimigosIntermediarios.push_back({3, 3 + i * 2, vidaIntermediario});
    }
    for (int i = 0; i < 9; i++)
    {
        inimigosFortes.push_back({2, 3 + i * 2, vidaForte});
    }
}

void inicializarIniimigosN2()
{
    int vidaIntermediario = (dificuldade == 1) ? 1 : (dificuldade == 2) ? 2 : 3;
    int vidaForte = (dificuldade == 1) ? 1 : (dificuldade == 2) ? 2 : 3;


    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 2; j++) {
        inimigosIntermediarios.push_back({3 + j, 3 + i * 2, vidaIntermediario});
        }
    }
    for (int i = 0; i < 9; i++)
    {
        inimigosFortes.push_back({2, 3 + i * 2, vidaForte});
    }
}

void inicializarBoss()
{
    Boss.push_back({4, 3, 5});
}

void inicializarIniimigosDesafio()
{
    for (int i = 0; i < 3; i++)
    {
        inimigosbasicos.push_back({4, 2 + i * 2, 1}); // sempre 1 de vida
    }

    for (int i = 0; i < 3; i++)
    {
        inimigosbasicos.push_back({4, 12 + i * 2, 1}); // sempre 1 de vida
    }

    for (int i = 0; i < 3; i++)
    {
        inimigosbasicos.push_back({4, 22 + i * 2, 1}); // sempre 1 de vida
    }

    int vidaIntermediario = (dificuldade == 1) ? 1 : (dificuldade == 2) ? 2 : 3;
    int vidaForte = (dificuldade == 1) ? 1 : (dificuldade == 2) ? 2 : 3;

    for (int i = 0; i < 3; i++)
    {
        inimigosIntermediarios.push_back({3, 2 + i * 2, vidaIntermediario});
    }

        for (int i = 0; i < 3; i++)
    {
        inimigosIntermediarios.push_back({3, 12 + i * 2, vidaIntermediario});
    }

        for (int i = 0; i < 3; i++)
    {
        inimigosIntermediarios.push_back({3, 22 + i * 2, vidaIntermediario});
    }

    for (int i = 0; i < 3; i++)
    {
        inimigosFortes.push_back({2, 2 + i * 2, vidaForte});
    }

        for (int i = 0; i < 3; i++)
    {
        inimigosFortes.push_back({2, 12 + i * 2, vidaForte});
    }

        for (int i = 0; i < 3; i++)
    {
        inimigosFortes.push_back({2, 22 + i * 2, vidaForte});
    }
}

void tentarDroparItem(int x, int y) {
    int chance = rand() % 100;
    if (chance < 30) { // 30% de chance de dropar
        TipoItem tipo = static_cast<TipoItem>(rand() % 6); // 0 a 5
        itensDropados.push_back({x + 1, y, tipo, chrono::steady_clock::now()});
    }
}

void atualizarItensDropados() {
    auto agora = chrono::steady_clock::now();

    for (size_t i = 0; i < itensDropados.size(); i++) {
        ItemDropado& it = itensDropados[i];
        it.x++; // o item desce

        if (it.x >= MAP_ALTURA - 2) {
            itensDropados.erase(itensDropados.begin() + i);
            i--;
        }
    }
}





void aplicarEfeitoItem(TipoItem tipo, int jogadorID) {
    jogador* j = (jogadorID == 1) ? &jogador1 : &jogador2;



    switch (tipo) {
        case VIDA_EXTRA:
            j->vida++;
            break;

        case AUMENTO_VELOCIDADE:
            if (jogadorID == 1) {
            delayMovimentoJogador1 = 50;
            efeitosJogador1.velocidade = true;
            efeitosJogador1.inicioVelocidade = chrono::steady_clock::now();
            }
            else
                delayMovimentoJogador2 = 50;
                efeitosJogador2.velocidade = true;
                efeitosJogador2.inicioVelocidade = chrono::steady_clock::now();
                break;

        case TIROS_EXTRA:
            if (jogadorID == 1) {
                efeitosJogador1.tirosExtra = true;
                efeitosJogador1.inicioTirosExtra = chrono::steady_clock::now();
            } else {
                efeitosJogador2.tirosExtra = true;
                efeitosJogador2.inicioTirosExtra = chrono::steady_clock::now();
                }
            break;

        case TIROS_MULTIPLOS:
            if (jogadorID == 1) {
                efeitosJogador1.tiroMultiplo = true;
                efeitosJogador1.inicioTiroMultiplo = chrono::steady_clock::now();
            } else {
                efeitosJogador2.tiroMultiplo = true;
                efeitosJogador2.inicioTiroMultiplo = chrono::steady_clock::now();
            }
            break;


        case PONTOS_EXTRA:
            j->pontuacao += 100;
            break;

        case CONGELAR_INIMIGOS:
            inimigosCongelados = true;
            inicioCongelamento = chrono::steady_clock::now();
            break;
    }
}

void inicializarBarreiras ()
{
    // primeira barreira
    barreirasDeProtecao.push_back({18, 6});
    barreirasDeProtecao.push_back({19, 5});
    barreirasDeProtecao.push_back({19, 6});
    barreirasDeProtecao.push_back({19, 7});

    // segunda barreira
    barreirasDeProtecao.push_back({18, 13});
    barreirasDeProtecao.push_back({19, 12});
    barreirasDeProtecao.push_back({19, 13});
    barreirasDeProtecao.push_back({19, 14});

    // terceira barreira
    barreirasDeProtecao.push_back({18, 20});
    barreirasDeProtecao.push_back({19, 19});
    barreirasDeProtecao.push_back({19, 20});
    barreirasDeProtecao.push_back({19, 21});

    // quarta barreira
    barreirasDeProtecao.push_back({18, 27});
    barreirasDeProtecao.push_back({19, 26});
    barreirasDeProtecao.push_back({19, 27});
    barreirasDeProtecao.push_back({19, 28});

}

int contarInimigosVivos()
{
    int total = 0;

    total += inimigosbasicos.size();
    total += inimigosIntermediarios.size();
    total += inimigosFortes.size();
    total += Boss.size();

    return total;
}



void moverInimigos()
{
    static auto ultimoMovimento = chrono::steady_clock::now(); // fica salvo entre chamadas
    auto agora = chrono::steady_clock::now();

    int vivos = contarInimigosVivos();
    int totalInimigos = (modoInfinito ? 27 : 28);
    int fatorVelocidade = 10 + (54 * vivos / totalInimigos); // mesma lógica do clock
    int intervaloMovimento = 20 * fatorVelocidade; // converte pra ms

    auto tempoDesdeUltimoMovimento = chrono::duration_cast<chrono::milliseconds>(agora - ultimoMovimento).count();

    if (tempoDesdeUltimoMovimento < intervaloMovimento)
        return; // ainda não é hora de mover

    ultimoMovimento = agora;

    if (inimigosCongelados) {
    auto agora = chrono::steady_clock::now();
    if (chrono::duration_cast<chrono::seconds>(agora - inicioCongelamento).count() >= 2)
        inimigosCongelados = false;
    return;
}


    // Verifica se algum inimigo bateu na parede
    for (auto &inimigo : inimigosbasicos)
    {
        int proximaPosicao = inimigo.y + direcaoInimigos;
        if (proximaPosicao < 0 || proximaPosicao >= MAP_LARGURA || mapaAtual[inimigo.x][proximaPosicao] == 1)
        {
            precisaDescer = true;
            direcaoInimigos *= -1;
            break;
        }
    }
    for (auto &inimigo : inimigosIntermediarios)
    {
        int proximaPosicao = inimigo.y + direcaoInimigos;
        if (proximaPosicao < 0 || proximaPosicao >= MAP_LARGURA || mapaAtual[inimigo.x][proximaPosicao] == 1)
        {
            precisaDescer = true;
            direcaoInimigos *= -1;
            break;
        }
    }
    for (auto &inimigo : inimigosFortes)
    {
        int proximaPosicao = inimigo.y + direcaoInimigos;
        if (proximaPosicao < 0 || proximaPosicao >= MAP_LARGURA || mapaAtual[inimigo.x][proximaPosicao] == 1)
        {
            precisaDescer = true;
            direcaoInimigos *= -1;
            break;
        }
    }
    for (auto &inimigo : Boss)
    {
        int proximaPosicao = inimigo.y + direcaoInimigos;
        if (proximaPosicao < 0 || proximaPosicao >= MAP_LARGURA || mapaAtual[inimigo.x][proximaPosicao] == 1)
        {
            precisaDescer = true;
            direcaoInimigos *= -1;
            break;
        }
    }


    // Move os inimigos
    for (auto &inimigo : inimigosbasicos)
        precisaDescer ? inimigo.x++ : inimigo.y += direcaoInimigos;

    for (auto &inimigo : inimigosIntermediarios)
        precisaDescer ? inimigo.x++ : inimigo.y += direcaoInimigos;

    for (auto &inimigo : inimigosFortes)
        precisaDescer ? inimigo.x++ : inimigo.y += direcaoInimigos;

    for (auto &inimigo : Boss)
        precisaDescer ? inimigo.x++ : inimigo.y += direcaoInimigos;


    precisaDescer = false; // resetar flag
}





void atiraProjetil1(int jogadorX, int jogadorY)
{
    int tirosExtras = efeitosJogador1.tirosExtra ? 1 : 0;

    bool mult = efeitosJogador1.tiroMultiplo;

    switch(tipoDeNave1) {
        case 1:
            projeteis.push_back({jogadorX - 1, jogadorY, 4});
            if (tirosExtras) {
                projeteis.push_back({jogadorX - 1, jogadorY + 1, 4});
            }
            break;
        case 2:
            projeteis.push_back({jogadorX - 1, jogadorY, 4});
            projeteis.push_back({jogadorX - 3, jogadorY, 4});
            if (tirosExtras) {
                projeteis.push_back({jogadorX - 1, jogadorY + 1, 4});
                projeteis.push_back({jogadorX - 3, jogadorY - 1, 4});
            }
            break;
        case 3:
            projeteis.push_back({jogadorX - 1, jogadorY, 4});
            projeteis.push_back({jogadorX - 1, jogadorY + 1, 4});
            if (tirosExtras) {
                projeteis.push_back({jogadorX - 2, jogadorY, 4});
                projeteis.push_back({jogadorX - 2, jogadorY - 1, 4});
            }
            break;
    }

    PlaySound(TEXT("tiroDoJogador.wav"), NULL, SND_FILENAME | SND_ASYNC);
}


void atiraProjetil2(int jogadorX, int jogadorY)
{
    int tirosExtras = efeitosJogador2.tirosExtra ? 1 : 0;

    bool mult = efeitosJogador2.tiroMultiplo;

    switch(tipoDeNave1) {
        case 1:
            projeteis.push_back({jogadorX - 1, jogadorY, 5});
            if (tirosExtras) {
                projeteis.push_back({jogadorX - 1, jogadorY + 1, 5});
            }
            break;
        case 2:
            projeteis.push_back({jogadorX - 1, jogadorY, 5});
            projeteis.push_back({jogadorX - 3, jogadorY, 5});
            if (tirosExtras) {
                projeteis.push_back({jogadorX - 1, jogadorY + 1, 5});
                projeteis.push_back({jogadorX - 3, jogadorY - 1, 5});
            }
            break;
        case 3:
            projeteis.push_back({jogadorX - 1, jogadorY, 5});
            projeteis.push_back({jogadorX - 1, jogadorY + 1, 5});
            if (tirosExtras) {
                projeteis.push_back({jogadorX - 2, jogadorY + 1, 5});
                projeteis.push_back({jogadorX - 2, jogadorY - 1, 5});
            }
            break;
    }

    PlaySound(TEXT("tiroDoJogador.wav"), NULL, SND_FILENAME | SND_ASYNC);
}



void atualizarProjeteis()
{
    for (int i = 0; i < projeteis.size(); i++)
    {
        projeteis[i].x--; // move o projétil para cima

        bool colidiu = false;

        // Verifica colisão com barreiras
        for (int b = 0; b < barreirasDeProtecao.size(); b++)
        {
            // Verifica se o projétil está na mesma posição ou adjacente à barreira
            if ((abs(projeteis[i].x - barreirasDeProtecao[b].x) < 1) &&
                (abs(projeteis[i].y - barreirasDeProtecao[b].y) < 1))
            {
                // Remove o projétil
                int tipoProjetil = projeteis[i].tipo; // Salva o tipo antes de remover
                projeteis.erase(projeteis.begin() + i);
                i--; // Ajusta o índice após remoção

                colidiu = true;
                continue;
            }
        }

        if (colidiu) continue;

        // Saiu da tela (topo)
        if (projeteis[i].x <= 0)
        {
            // Atualiza flags antes de remover
            if (projeteis[i].tipo == 4) { // Jogador 1
            }
            else if (projeteis[i].tipo == 5) { // Jogador 2
            }
            projeteis.erase(projeteis.begin() + i);
            i--;
            continue;
        }

        // Colisão com inimigos básicos
        for (int j = 0; j < inimigosbasicos.size(); j++)
        {
            if (projeteis[i].x == inimigosbasicos[j].x && abs(projeteis[i].y - inimigosbasicos[j].y) <= 1)
        {
        inimigosbasicos[j].vida--; // reduz vida
        if (inimigosbasicos[j].vida <= 0)
        {
        //inimigo eliminado
        explosaoX = inimigosbasicos[j].x;
        explosaoY = inimigosbasicos[j].y;
        tempoExplosao = std::chrono::steady_clock::now();
        explosaoAtiva = true;
        PlaySound(TEXT("explosaoInimigo.wav"), NULL, SND_FILENAME | SND_ASYNC);
        tentarDroparItem(inimigosbasicos[j].x, inimigosbasicos[j].y);
        inimigosbasicos.erase(inimigosbasicos.begin() + j);
        mensagemCombate = "Eliminou um inimigo!";
        jogador1.pontuacao += 10;
    }
        projeteis.erase(projeteis.begin() + i);
    i--;
    break;
}

        }

        for (int j = 0; j < inimigosIntermediarios.size(); j++)
        {
            if (projeteis[i].x == inimigosIntermediarios[j].x && abs(projeteis[i].y - inimigosIntermediarios[j].y) <= 1)
{
    inimigosIntermediarios[j].vida--; // reduz vida
    if (inimigosIntermediarios[j].vida <= 0)
    {
        // inimigo eliminado
        explosaoX = inimigosIntermediarios[j].x;
        explosaoY = inimigosIntermediarios[j].y;
        tempoExplosao = std::chrono::steady_clock::now();
        explosaoAtiva = true;
        PlaySound(TEXT("explosaoInimigo.wav"), NULL, SND_FILENAME | SND_ASYNC);
        tentarDroparItem(inimigosIntermediarios[j].x, inimigosIntermediarios[j].y);
        inimigosIntermediarios.erase(inimigosIntermediarios.begin() + j);
        mensagemCombate = "Eliminou um inimigo!";
        jogador1.pontuacao += 20;
    }
    projeteis.erase(projeteis.begin() + i);
    i--;
    break;
}
        }

        for (int j = 0; j < inimigosFortes.size(); j++)
        {
            if (projeteis[i].x == inimigosFortes[j].x && abs(projeteis[i].y - inimigosFortes[j].y) <= 1)
{
    inimigosFortes[j].vida--; // reduz vida
    if (inimigosFortes[j].vida <= 0)
    {
        // inimigo eliminado
        explosaoX = inimigosFortes[j].x;
        explosaoY = inimigosFortes[j].y;
        tempoExplosao = std::chrono::steady_clock::now();
        explosaoAtiva = true;
        PlaySound(TEXT("explosaoInimigo.wav"), NULL, SND_FILENAME | SND_ASYNC);
        tentarDroparItem(inimigosFortes[j].x, inimigosFortes[j].y);
        inimigosFortes.erase(inimigosFortes.begin() + j);
        mensagemCombate = "Eliminou um inimigo!";
        jogador1.pontuacao += 30;
    }
    projeteis.erase(projeteis.begin() + i);
    i--;
    break;
}
        }
        for (int j = 0; j < Boss.size(); j++)
        {
            if (projeteis[i].x == Boss[j].x && abs(projeteis[i].y - Boss[j].y) <= 1)
{
    Boss[j].vida--; // reduz vida
    if (Boss[j].vida <= 0)
    {
        // inimigo eliminado
        explosaoX = Boss[j].x;
        explosaoY = Boss[j].y;
        tempoExplosao = std::chrono::steady_clock::now();
        explosaoAtiva = true;
        PlaySound(TEXT("explosaoInimigo.wav"), NULL, SND_FILENAME | SND_ASYNC);
        Boss.erase(Boss.begin() + j);
        mensagemCombate = "Eliminou um inimigo!";
        jogador1.pontuacao += 100;
    }
    projeteis.erase(projeteis.begin() + i);
    i--;
    break;
}

        }
    }
}


void inimigoAtira()
{
    if (projetilInimigosExiste)
        return;


    // ---------- Inimigos Básicos (prioridade 1) ----------
    if (!inimigosbasicos.empty())
    {
        int idx = rand() % inimigosbasicos.size();
        inimigos& atirador = inimigosbasicos[idx];

        projeteisInimigos.push_back({atirador.x + 1, atirador.y, 3});
        PlaySound(TEXT("tiroDoInimigo.wav"), NULL, SND_FILENAME | SND_ASYNC);
        projetilInimigosExiste = true;
        return;
    }

    // ---------- Inimigos Intermediários (prioridade 2) ----------
    if (!inimigosIntermediarios.empty())
    {
        int idx = rand() % inimigosIntermediarios.size();
        inimigos& atirador = inimigosIntermediarios[idx];

        projeteisInimigos.push_back({atirador.x + 1, atirador.y, 2});
        if (atirador.y + 1 < MAP_LARGURA)
            projeteisInimigos.push_back({atirador.x + 1, atirador.y + 1, 2});
        PlaySound(TEXT("tiroDoInimigo.wav"), NULL, SND_FILENAME | SND_ASYNC);
        projetilInimigosExiste = true;
        return;
    }

    // ---------- Inimigos Fortes (prioridade 3) ----------
    if (!inimigosFortes.empty())
    {
        int idx = rand() % inimigosFortes.size();
        inimigos& atirador = inimigosFortes[idx];

        // tiro duplo na vertical
        projeteisInimigos.push_back({atirador.x + 1, atirador.y, 1});
         projeteisInimigos.push_back({atirador.x + 3, atirador.y, 1});
        PlaySound(TEXT("tiroDoInimigo.wav"), NULL, SND_FILENAME | SND_ASYNC);
        projetilInimigosExiste = true;
        return;
    }

    // ---------- Boss ----------
    if (!Boss.empty())
    {
        int idx = rand() % Boss.size();
        inimigos& atirador = Boss[idx];

        projeteisInimigos.push_back({atirador.x + 1, atirador.y, 1});
        projeteisInimigos.push_back({atirador.x + 1, atirador.y - 1, 1});

        projeteisInimigos.push_back({atirador.x + 1 + 2, atirador.y, 1});
        projeteisInimigos.push_back({atirador.x + 1 + 2, atirador.y - 1, 1});
        PlaySound(TEXT("tiroDoInimigo.wav"), NULL, SND_FILENAME | SND_ASYNC);


        projetilInimigosExiste = true;
        return;
    }

}


void atualizarProjeteisInimigos()
{
    for (int i = 0; i < projeteisInimigos.size(); i++)
    {
        projeteisInimigos[i].x++; // Desce


        int novaX = projeteisInimigos[i].x + 1;
        // Verifica colisão com barreira
        if (novaX < MAP_ALTURA && mapaAtual[novaX][projeteisInimigos[i].y] == 2)
        {
            projeteisInimigos.erase(projeteisInimigos.begin() + i);
            i--;
            projetilInimigosExiste = false;
            continue;
        }

        // Saiu da tela
        if (projeteisInimigos[i].x >= MAP_ALTURA - 1)
        {
            projeteisInimigos.erase(projeteisInimigos.begin() + i);
            i--;
            projetilInimigosExiste = false;
            continue;
        }

        for (int b = 0; b < barreirasDeProtecao.size(); b++)
        {
            if (barreirasDeProtecao[b].x == novaX &&
            barreirasDeProtecao[b].y == projeteisInimigos[i].y)
            {
                barreirasDeProtecao[b].vida--;
                if (barreirasDeProtecao[b].vida <= 0)
                {
                    barreirasDeProtecao.erase(barreirasDeProtecao.begin() + b);
                }
            projeteisInimigos.erase(projeteisInimigos.begin() + i);
            i--;
            break;
            }
        }

        // Acertou o jogador1
        if (jogador1.vivo && projeteisInimigos[i].x == jogador1.x && abs(projeteisInimigos[i].y - jogador1.y) <= 1)

        {
            jogador1.vida--;
            mensagemCombate = "Ai! Levou tiro inimigo!";
            projeteisInimigos.erase(projeteisInimigos.begin() + i);
            ultimoDanoJogador1 = chrono::steady_clock::now();
            jogador1Piscando = true;

            i--;
            projetilInimigosExiste = false;

            if(jogador1.vida <= 0)
            {
                jogador1.vivo = false;
                mensagemCombate = "Player1 morreu";
            }
            continue;
        }
        if (jogador2.vivo && projeteisInimigos[i].x == jogador2.x && (projeteisInimigos[i].y - jogador2.y) <= 1)

        {
            jogador2.vida--;
            mensagemCombate = "Ai! Levou tiro inimigo!";
            projeteisInimigos.erase(projeteisInimigos.begin() + i);
            i--;
            ultimoDanoJogador2 = chrono::steady_clock::now();
            jogador2Piscando = true;

            projetilInimigosExiste = false;

            if(jogador2.vida <= 0)
            {
                jogador2.vivo = false;
                mensagemCombate = "Player2 morreu";
            }
            continue;
        }
    }
}

void detectarColisaoProjeteis()
{
    vector<int> indicesProjJogador;
    vector<int> indicesProjInimigo;

    for (int i = 0; i < projeteis.size(); i++) {
        for (int j = 0; j < projeteisInimigos.size(); j++) {
            if (projeteis[i].x == projeteisInimigos[j].x &&
                projeteis[i].y == projeteisInimigos[j].y) {

                // Ativa explosão na posição da colisão
                explosaoX = projeteis[i].x;
                explosaoY = projeteis[i].y;
                explosaoAtiva = true;
                tempoExplosao = chrono::steady_clock::now();

                indicesProjJogador.push_back(i);
                indicesProjInimigo.push_back(j);
                mensagemCombate = "Projeteis colidiram!                            ";
                break;
            }
        }
    }

    sort(indicesProjJogador.rbegin(), indicesProjJogador.rend());
    sort(indicesProjInimigo.rbegin(), indicesProjInimigo.rend());

    for (int idx : indicesProjJogador)
        if (idx < projeteis.size())
            projeteis.erase(projeteis.begin() + idx);
    for (int idx : indicesProjInimigo)
        if (idx < projeteisInimigos.size())
            projeteisInimigos.erase(projeteisInimigos.begin() + idx);

    projetilInimigosExiste = !projeteisInimigos.empty();
}



void modoMultiplayerMenu()
{
    cout << "Deseja jogar multiplayer? (s/n): ";
    char escolhaModo;
    cin >> escolhaModo;
    modoMultiplayer = (escolhaModo == 's' || escolhaModo == 'S');

}

void controlarBotJogador1() {
    int alvoY = -1;
    int alvoX = -1;

    // Procurar inimigo mais baixo e próximo
    for (int i = 0; i < inimigosbasicos.size(); i++) {
        if (alvoX == -1 || inimigosbasicos[i].x > alvoX ||
            (inimigosbasicos[i].x == alvoX && abs(inimigosbasicos[i].y - jogador1.y) < abs(alvoY - jogador1.y))) {
            alvoX = inimigosbasicos[i].x;
            alvoY = inimigosbasicos[i].y;
        }
    }
    for (int i = 0; i < inimigosIntermediarios.size(); i++) {
        if (alvoX == -1 || inimigosIntermediarios[i].x > alvoX ||
            (inimigosIntermediarios[i].x == alvoX && abs(inimigosIntermediarios[i].y - jogador1.y) < abs(alvoY - jogador1.y))) {
            alvoX = inimigosIntermediarios[i].x;
            alvoY = inimigosIntermediarios[i].y;
        }
    }
    for (int i = 0; i < inimigosFortes.size(); i++) {
        if (alvoX == -1 || inimigosFortes[i].x > alvoX ||
            (inimigosFortes[i].x == alvoX && abs(inimigosFortes[i].y - jogador1.y) < abs(alvoY - jogador1.y))) {
            alvoX = inimigosFortes[i].x;
            alvoY = inimigosFortes[i].y;
        }
    }
    for (int i = 0; i < Boss.size(); i++) {
        if (alvoX == -1 || Boss[i].x > alvoX ||
            (Boss[i].x == alvoX && abs(Boss[i].y - jogador1.y) < abs(alvoY - jogador1.y))) {
            alvoX = Boss[i].x;
            alvoY = Boss[i].y;
        }
    }
bool temBarreira = false;

    // Movimentação: se adianta um pouco
    if (alvoY != -1) {
        if (alvoY < jogador1.y) jogador1.y--;
        else if (alvoY > jogador1.y) jogador1.y++;
    }

    // Checar se tem barreira à frente

    for (int i = 0; i < barreirasDeProtecao.size(); i++) {
        if (barreirasDeProtecao[i].x < jogador1.x && barreirasDeProtecao[i].y == jogador1.y && barreirasDeProtecao[i].vida > 0) {
            temBarreira = true;
            break;
        }
    }

    // Atirar se não tiver barreira e sem projétil
    if (!temBarreira && !existeProjetilTipo(4)) {
        atiraProjetil1(jogador1.x, jogador1.y);
    }
}



void controlesJogadores()
{
    if (jogador1IA) {
        controlarBotJogador1();
    }
    else if (_kbhit())
    {
        char tecla = getch();

        // Detecta teclas especiais (como setas)
        if (tecla == -32 || tecla == 224)
        {
            tecla = getch();
            switch (tecla)
            {
            case 75: // ← Jogador 2
        {
        auto agora2 = chrono::steady_clock::now();
        auto tempo = chrono::duration_cast<chrono::milliseconds>(agora2 - ultimoMovimentoJogador2).count();
        if (tempo >= delayMovimentoJogador2) {
        if (jogador2.y > 0 && mapaAtual[jogador2.x][jogador2.y - 1] == 0)
        jogador2.y--;
        ultimoMovimentoJogador2 = agora2;
        }
        break;
        }
            case 77: // → Jogador 2
{
auto agora2 = chrono::steady_clock::now();
auto tempo = chrono::duration_cast<chrono::milliseconds>(agora2 - ultimoMovimentoJogador2).count();

if (tempo >= delayMovimentoJogador2) {
if (jogador2.y < MAP_LARGURA - 1 && mapaAtual[jogador2.x][jogador2.y + 1] == 0)
jogador2.y++;
ultimoMovimentoJogador2 = agora2;
}
break;
}
            }
        }
        else
        {
            switch (tecla)
            {
                // Jogador 1 - A / D
            case 'a':
            case 'A':
{
auto agora = chrono::steady_clock::now();
auto tempo = chrono::duration_cast<chrono::milliseconds>(agora - ultimoMovimentoJogador1).count();
if (tempo >= delayMovimentoJogador1) {
if (jogador1.y > 0 && mapaAtual[jogador1.x][jogador1.y - 1] == 0)
jogador1.y--;
ultimoMovimentoJogador1 = agora;
}
break;
}

            case 'd':
            case 'D':
{
auto agora = chrono::steady_clock::now();
auto tempo = chrono::duration_cast<chrono::milliseconds>(agora - ultimoMovimentoJogador1).count();

if (tempo >= delayMovimentoJogador1) {
if (jogador1.y < MAP_LARGURA - 1 && mapaAtual[jogador1.x][jogador1.y + 1] == 0)
jogador1.y++;
ultimoMovimentoJogador1 = agora;
}
break;
}

                // Jogador 1 - Espaço
                case ' ':
                    if (!existeProjetilTipo(4)) {
                    atiraProjetil1(jogador1.x, jogador1.y);
                }
                break;

                case '/':
                if (modoMultiplayer && jogador2.vivo && !existeProjetilTipo(5)) {
                    atiraProjetil2(jogador2.x, jogador2.y);
                }
                break;

            }
        }
    }
}



int main()
{

    atualizarData();
    HWND hWnd = GetConsoleWindow();
    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    style &= ~WS_MAXIMIZEBOX;     // Remove botão de maximizar
    style &= ~WS_THICKFRAME;      // Remove redimensionamento
    SetWindowLong(hWnd, GWL_STYLE, style);



    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(out, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(out, &cursorInfo);

    short int CX=0, CY=0;
    COORD coord;
    coord.X = CX;
    coord.Y = CY;

    bool perdeuPorinvasao = false;

    int escolha = menuInicio();


    system("cls");


    jogador1 = {22, 10, 3, 0};
    if (modoMultiplayer) {
        jogador2 = {22, 14, 3, 0};
    }
    else {
    jogador2.vivo = false;
    }

    time_t tempoInicial = time(NULL);
    auto ultimoTiroInimigoChrono = chrono::steady_clock::now();
    auto ultimoUpdateProjInimigo = chrono::steady_clock::now();
    auto ultimoUpdateProjJogador = chrono::steady_clock::now();


    // Após escolha no menu
    if (modoInfinito) {
        nivel = 0;
        inicializarIniimigosDesafio();
    } else if (nivel == 1) {
        inicializarIniimigosN1();
    } else if (nivel == 2) {
        inicializarIniimigosN2();
    } else if (nivel == 3) {
        inicializarBoss();
    }

    inicializarBarreiras();
    carregarMapa("Mapa.txt", mapaAtual);


    ultimoUpdateItensDropados = chrono::steady_clock::now();

    while(true)
    {

        ///Posiciona a escrita no iicio do console
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);

        auto agora = chrono::steady_clock::now();

        if (efeitosJogador1.tirosExtra &&
            chrono::duration_cast<chrono::seconds>(agora - efeitosJogador1.inicioTirosExtra).count() >= 6) {
            efeitosJogador1.tirosExtra = false;
        }

        if (efeitosJogador1.tiroMultiplo &&
        chrono::duration_cast<chrono::seconds>(agora - efeitosJogador1.inicioTiroMultiplo).count() >= 6) {
        efeitosJogador1.tiroMultiplo = false;
        }


        if (efeitosJogador1.velocidade &&
            chrono::duration_cast<chrono::seconds>(agora - efeitosJogador1.inicioVelocidade).count() >= 8) {
            delayMovimentoJogador1 = 100;  // valor original
            efeitosJogador1.velocidade = false;
        }
        if (efeitosJogador2.velocidade &&
            chrono::duration_cast<chrono::seconds>(agora - efeitosJogador2.inicioVelocidade).count() >= 8) {
            delayMovimentoJogador2 = 100;  // valor original
            efeitosJogador2.velocidade = false;
        }


        // Disparo dos inimigos a cada 2 segundos
        auto tempoDesdeUltimoTiro = chrono::duration_cast<chrono::seconds>(agora - ultimoTiroInimigoChrono).count();
        if (tempoDesdeUltimoTiro >= 2)
        {
            inimigoAtira();
            ultimoTiroInimigoChrono = agora;
        }
        // Atualização dos projéteis inimigos a cada 150ms
        auto tempoDesdeUltimaAtualizacaoProj = chrono::duration_cast<chrono::milliseconds>(agora - ultimoUpdateProjInimigo).count();
        if (tempoDesdeUltimaAtualizacaoProj >= 150)
        {
            atualizarProjeteisInimigos();
            detectarColisaoProjeteis();
            ultimoUpdateProjInimigo = agora;
        }

                // Itens dropados descem
            auto agoraitem = chrono::steady_clock::now();
    auto tempoDesdeUltimoUpdateItens = chrono::duration_cast<chrono::milliseconds>(agora - ultimoUpdateItensDropados).count();

    if (tempoDesdeUltimoUpdateItens >= 250) { // 300 ms entre cada queda — você pode ajustar

        atualizarItensDropados();
        for (int i = 0; i < itensDropados.size(); i++) {
    itensDropados[i].x++;

    bool coletado = false;  // <- agora é local, reinicia a cada item

    // Jogador 1 coleta
    if (jogador1.vivo &&
    itensDropados[i].x == jogador1.x &&
    itensDropados[i].y == jogador1.y) {
    aplicarEfeitoItem(itensDropados[i].tipo, 1);

    string nomeItem[] = {
        "Vida Extra", "Velocidade", "Tiros Extra",
        "Tiros Multiplos", "Pontos Extras", "Congelamento"
    };
    mensagemCombate = "J1 pegou: " + nomeItem[itensDropados[i].tipo];

    coletado = true;
}


    // Jogador 2 coleta
    else if (modoMultiplayer && jogador2.vivo &&
             itensDropados[i].x == jogador2.x &&
             itensDropados[i].y == jogador2.y) {
            aplicarEfeitoItem(itensDropados[i].tipo, 2);
            string nomeItem[] = {
            "Vida Extra", "Velocidade", "Tiros Extra", "Tiros Multiplos", "Pontos Extras", "Congelamento"
            };
            mensagemCombate = "J2 pegou: " + nomeItem[itensDropados[i].tipo];
            coletado = true;
    }

    if (coletado) {
        itensDropados.erase(itensDropados.begin() + i);
        i--;
        continue;
    }

    if (itensDropados[i].x >= MAP_ALTURA) {
        itensDropados.erase(itensDropados.begin() + i);
        i--;
    }
}


    ultimoUpdateItensDropados = agora;
}


        agora = chrono::steady_clock::now();
        auto tempoDesdeUltimoDesgaste = chrono::duration_cast<chrono::seconds>(agora - ultimoDesgasteBarreiras).count();

        if (tempoDesdeUltimoDesgaste >= 60)
        {
            for (auto &b : barreirasDeProtecao)
            {
                if (b.vida > 0)
                b.vida--;
            }
            ultimoDesgasteBarreiras = agora;
        }

        controlesJogadores();
        moverInimigos();

        ///Imprime o jogo: mapa e personagem.
        for(int i=0; i< MAP_ALTURA; i++)
        {
            for(int j=0; j< MAP_LARGURA; j++)
            {
                bool desenhou = false;

                    auto agora = std::chrono::steady_clock::now();
                if (explosaoAtiva && i == explosaoX && j == explosaoY &&
                    std::chrono::duration_cast<std::chrono::milliseconds>(agora - tempoExplosao).count() < 300)
                {
                    std::cout << "*"; // Desenha a explosão vermelha na posição da nave atingida
                    continue;  // Pula o resto do desenho para esse ponto
                }
                else if (explosaoAtiva &&
                         std::chrono::duration_cast<std::chrono::milliseconds>(agora - tempoExplosao).count() >= 300)
                {
                    explosaoAtiva = false; // Desliga a explosão depois de 300ms
                }

                // Primeiro: verifica projeteis inimigos
                for (auto &projI : projeteisInimigos)
                {
                    if (projI.x == i && projI.y == j)
                    {
                        switch (projI.tipo)
                        {
                        case 1: // básico
                            cout << "\033[31m" << "|" << "\033[0m"; // vermelho
                            break;
                        case 2: // intermediário
                            cout << "\033[33m" << "|" << "\033[0m"; // amarelo
                            break;
                        case 3: // forte
                            cout << "\033[35m" << "|" << "\033[0m"; // magenta
                            break;
                        default: // quabndo ha colisao
                            cout << "\033[37m" << "|" << "\033[0m"; // branco
                            break;
                        }
                        desenhou = true;
                        break;
                    }
                }
                if (desenhou) continue;

                // Depois: projeteis do jogador
                for (auto &projP : projeteis)
                {
                    if (projP.x == i && projP.y == j)
                    {
                        switch (projP.tipo)
                        {
                        case 4: // básico
                            cout << "\033[32m" << "|" << "\033[0m"; // verde
                            break;
                        case 5:
                            cout << "\033[36m" << "|" << "\033[0m"; // ciano
                            break;
                        }
                        desenhou = true;
                        break;
                    }
                }
                if (desenhou) continue;


                if (jogador1.vivo && i == jogador1.x && j == jogador1.y)
                {
                    auto agora = chrono::steady_clock::now();
                    if (jogador1Piscando &&
                            chrono::duration_cast<chrono::milliseconds>(agora - ultimoDanoJogador1).count() < 300)
                    {
                        cout << "\033[37m" << 'W' << "\x1b[0m"; // branco
                    }
                    else
                    {
                        cout << "\033[32m" << 'W' << "\x1b[0m"; // verde
                        jogador1Piscando = false;
                    }
                }
                else if (jogador2.vivo && modoMultiplayer && i == jogador2.x && j == jogador2.y)
                {
                    auto agora = chrono::steady_clock::now();
                    if (jogador2Piscando &&
                            chrono::duration_cast<chrono::milliseconds>(agora - ultimoDanoJogador2).count() < 300)
                    {
                        cout << "\033[37m" << 'X' << "\x1b[0m"; // branco
                    }
                    else
                    {
                        cout << "\033[36m" << 'X' << "\x1b[0m"; // ciano
                        jogador2Piscando = false;
                    }
                }


                else if (ehInimigo(i, j)) cout << "\033[35m" << 'M' << "\x1b[0m";
                else if (ehInimigoIntermediario(i, j)) cout << "\033[33m" << 'M' << "\x1b[0m";
                else if (ehInimigoForte(i, j)) cout << "\033[31m" << 'M' << "\x1b[0m";
                else if (ehBoss(i, j)) cout << "\033[31m" << 'A' << "\x1b[0m";
                else if (ehBarreira(i, j))
                {
                    for (auto &b : barreirasDeProtecao)
                    {
                        if (b.x == i && b.y == j)
                    {
                        if (b.vida == 3)
                        cout << "\033[92m" << "#" << "\033[0m"; // Verde
                        else if (b.vida == 2)
                        cout << "\033[93m" << "+" << "\033[0m"; // Amarelo
                        else if (b.vida == 1)
                        cout << "\033[91m" << "-" << "\033[0m"; // Vermelho
                        else if (b.vida <= 0)
                        cout << " ";
                        break;
                    }
                    }
                }
                else if (ehitemdrop(i, j)) cout << "\033[31m" << 'A' << "\x1b[0m";
                else
                {
                    switch (mapaAtual[i][j])
                    {
                    case 0:
                        cout<<" ";
                        break; //caminho
                    case 1:
                        cout<<char(219);
                        break; //parede
                    } //fim switch
                }
            }
            cout<<"\n";
        } //fim for mapa

        auto tempoDesdeUltimoProjJogador = chrono::duration_cast<chrono::milliseconds>(agora - ultimoUpdateProjJogador).count();
        if (tempoDesdeUltimoProjJogador >= 160) // 150 ms entre cada "subida" dos projéteis
        {
            atualizarProjeteis();
            detectarColisaoProjeteis();
            ultimoUpdateProjJogador = agora;
        }

        cout << "\n";

        for (auto &inimigos : inimigosbasicos)
        {
            if (inimigos.x >= 17)   // ou qualquer linha que represente o "fim do mapa"
            {
                perdeuPorinvasao = true;
            }
        }

        for (auto &inimigos : inimigosIntermediarios)
        {
            if (inimigos.x >= 17)   // ou qualquer linha que represente o "fim do mapa"
            {
                perdeuPorinvasao = true;
            }
        }

        for (auto &inimigos : inimigosFortes)
        {
            if (inimigos.x >= 17)   // ou qualquer linha que represente o "fim do mapa"
            {
                perdeuPorinvasao = true;
            }
        }

        if ((!jogador1.vivo && (!jogador2.vivo || !modoMultiplayer)) || perdeuPorinvasao)
        {
            system("cls");
            cout << "Invadiram sua base ou vc perdeu todas as vidas!" << endl;
            cout << "Pressione qualquer tecla para avancar";
            PlaySound(TEXT("Derrota.wav"), NULL, SND_FILENAME | SND_ASYNC);
            jogadorVenceu = "NAO";
            while (!_kbhit())
            {

            }
            break;
        }
        if (!modoInfinito && contarInimigosVivos() == 0) {
                nivel++;
            if (nivel == 2) {
                inimigosbasicos.clear();
                inimigosIntermediarios.clear();
                inimigosFortes.clear();
                Boss.clear();
                barreirasDeProtecao.clear();
                itensDropados.clear();
                inicializarIniimigosN2();
                inicializarBarreiras();
                carregarMapa("Mapa.txt", mapaAtual);
                mensagemCombate = "Indo para o Nivel 2!";
            } else if (nivel == 3) {
                inimigosbasicos.clear();
                inimigosIntermediarios.clear();
                inimigosFortes.clear();
                Boss.clear();
                barreirasDeProtecao.clear();
                itensDropados.clear();
                inicializarBoss();
                inicializarBarreiras();
                carregarMapa("Mapa.txt", mapaAtual);
                mensagemCombate = "Chegou no Boss!";
            } else {
                jogadorVenceu = "Sim";

                mensagemCombate = "Você venceu!";
                break;
            }
        }

    desenharHUDLateral(tempoInicial);


    } //fim do laco do jogo

    time_t tempoFinal = time(NULL);
    int tempoTotal = difftime(tempoFinal, tempoInicial);

    string nomeJogador;
    system("cls");
    cout << "\nDigite seu nome para salvar no ranking: ";
    cin >> nomeJogador;

    salvarPontuacao(nomeJogador, jogador1.pontuacao, tempoTotal);
    mostrarRanking();

    return 0;
} //fim main
