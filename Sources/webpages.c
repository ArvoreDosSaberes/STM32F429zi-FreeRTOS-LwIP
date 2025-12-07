/**
 * @file webpages.c
 * @brief Páginas web embarcadas para o servidor HTTP.
 *
 * Este arquivo contém as páginas HTML da landing page e página 404
 * embarcadas diretamente no firmware. O design usa fundo laranja
 * com detalhes em verde e marrom, letras brancas e botões em laranja.
 *
 * @note As páginas são armazenadas em flash para economizar RAM.
 */

#include "webpages.h"
#include <stdint.h>
#include <string.h>

/*-----------------------------------------------------------------------------
 * Definições de Páginas HTML
 *----------------------------------------------------------------------------*/

/**
 * @brief Landing page HTML (index.html).
 *
 * Página principal do projeto STM32F429ZI-FreeRTOS-LwIP com:
 * - Fundo laranja (#FF6B35)
 * - Detalhes em verde (#2E8B57) e marrom (#8B4513)
 * - Texto em branco
 * - Botões estilizados em laranja
 */
static const char indexHtml[] =
"<!DOCTYPE html>\n"
"<html lang=\"pt-BR\">\n"
"<head>\n"
"    <meta charset=\"UTF-8\">\n"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"    <title>STM32F429ZI - FreeRTOS + LwIP</title>\n"
"    <style>\n"
"        * {\n"
"            margin: 0;\n"
"            padding: 0;\n"
"            box-sizing: border-box;\n"
"        }\n"
"        body {\n"
"            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n"
"            background: linear-gradient(135deg, #FF6B35 0%, #FF8C42 50%, #FF6B35 100%);\n"
"            min-height: 100vh;\n"
"            color: #FFFFFF;\n"
"            display: flex;\n"
"            flex-direction: column;\n"
"            align-items: center;\n"
"            padding: 20px;\n"
"        }\n"
"        .container {\n"
"            max-width: 900px;\n"
"            width: 100%;\n"
"            text-align: center;\n"
"        }\n"
"        .header {\n"
"            background: rgba(139, 69, 19, 0.3);\n"
"            border: 3px solid #2E8B57;\n"
"            border-radius: 20px;\n"
"            padding: 30px;\n"
"            margin-bottom: 30px;\n"
"            box-shadow: 0 8px 32px rgba(0,0,0,0.2);\n"
"        }\n"
"        .logo {\n"
"            font-size: 3em;\n"
"            margin-bottom: 10px;\n"
"        }\n"
"        h1 {\n"
"            font-size: 2.2em;\n"
"            margin-bottom: 10px;\n"
"            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);\n"
"        }\n"
"        .subtitle {\n"
"            font-size: 1.2em;\n"
"            color: #2E8B57;\n"
"            font-weight: bold;\n"
"            background: rgba(255,255,255,0.9);\n"
"            padding: 8px 20px;\n"
"            border-radius: 25px;\n"
"            display: inline-block;\n"
"            margin-top: 10px;\n"
"        }\n"
"        .content {\n"
"            background: rgba(139, 69, 19, 0.2);\n"
"            border: 2px solid #8B4513;\n"
"            border-radius: 15px;\n"
"            padding: 30px;\n"
"            margin-bottom: 30px;\n"
"            text-align: left;\n"
"        }\n"
"        .content h2 {\n"
"            color: #2E8B57;\n"
"            background: rgba(255,255,255,0.9);\n"
"            padding: 10px 20px;\n"
"            border-radius: 10px;\n"
"            display: inline-block;\n"
"            margin-bottom: 20px;\n"
"        }\n"
"        .content p {\n"
"            font-size: 1.1em;\n"
"            line-height: 1.8;\n"
"            margin-bottom: 15px;\n"
"        }\n"
"        .features {\n"
"            display: grid;\n"
"            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));\n"
"            gap: 20px;\n"
"            margin: 25px 0;\n"
"        }\n"
"        .feature-card {\n"
"            background: rgba(46, 139, 87, 0.3);\n"
"            border: 2px solid #2E8B57;\n"
"            border-radius: 10px;\n"
"            padding: 20px;\n"
"            text-align: center;\n"
"        }\n"
"        .feature-card h3 {\n"
"            color: #FFE4B5;\n"
"            margin-bottom: 10px;\n"
"        }\n"
"        .buttons {\n"
"            display: flex;\n"
"            flex-wrap: wrap;\n"
"            justify-content: center;\n"
"            gap: 15px;\n"
"            margin-top: 30px;\n"
"        }\n"
"        .btn {\n"
"            display: inline-flex;\n"
"            align-items: center;\n"
"            gap: 10px;\n"
"            background: linear-gradient(135deg, #FF6B35, #FF8C42);\n"
"            color: white;\n"
"            text-decoration: none;\n"
"            padding: 15px 30px;\n"
"            border-radius: 30px;\n"
"            font-size: 1.1em;\n"
"            font-weight: bold;\n"
"            border: 3px solid #8B4513;\n"
"            box-shadow: 0 4px 15px rgba(0,0,0,0.3);\n"
"            transition: all 0.3s ease;\n"
"        }\n"
"        .btn:hover {\n"
"            transform: translateY(-3px);\n"
"            box-shadow: 0 6px 20px rgba(0,0,0,0.4);\n"
"            background: linear-gradient(135deg, #FF8C42, #FFA500);\n"
"        }\n"
"        .btn-github {\n"
"            border-color: #2E8B57;\n"
"        }\n"
"        .btn-site {\n"
"            border-color: #8B4513;\n"
"        }\n"
"        .btn-youtube {\n"
"            border-color: #8B0000;\n"
"        }\n"
"        .footer {\n"
"            margin-top: auto;\n"
"            padding-top: 30px;\n"
"            color: rgba(255,255,255,0.8);\n"
"            font-size: 0.9em;\n"
"        }\n"
"        .footer a {\n"
"            color: #FFE4B5;\n"
"        }\n"
"        .tech-badges {\n"
"            display: flex;\n"
"            flex-wrap: wrap;\n"
"            justify-content: center;\n"
"            gap: 10px;\n"
"            margin-top: 20px;\n"
"        }\n"
"        .badge {\n"
"            background: #2E8B57;\n"
"            color: white;\n"
"            padding: 5px 15px;\n"
"            border-radius: 15px;\n"
"            font-size: 0.9em;\n"
"            border: 2px solid #8B4513;\n"
"        }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"    <div class=\"container\">\n"
"        <div class=\"header\">\n"
"            <div class=\"logo\">&#x1F4BB; &#x1F333;</div>\n"
"            <h1>STM32F429ZI Template</h1>\n"
"            <h2>FreeRTOS + LwIP HTTP Server</h2>\n"
"            <span class=\"subtitle\">Arvore dos Saberes</span>\n"
"            <div class=\"tech-badges\">\n"
"                <span class=\"badge\">STM32F429ZI</span>\n"
"                <span class=\"badge\">FreeRTOS v11.2</span>\n"
"                <span class=\"badge\">LwIP v2.2</span>\n"
"                <span class=\"badge\">ARM Cortex-M4</span>\n"
"            </div>\n"
"        </div>\n"
"\n"
"        <div class=\"content\">\n"
"            <h2>Sobre o Projeto</h2>\n"
"            <p>\n"
"                Este projeto tem como objetivo <strong>ensinar e facilitar o desenvolvimento</strong>\n"
"                com microcontroladores da familia STM32, especificamente o STM32F429ZI.\n"
"            </p>\n"
"            <p>\n"
"                Oferecemos a comunidade <strong>templates prontos para uso</strong>, permitindo\n"
"                que desenvolvedores deem os primeiros passos com o STM32 de forma rapida e\n"
"                eficiente, sem a necessidade de configurar tudo do zero.\n"
"            </p>\n"
"\n"
"            <div class=\"features\">\n"
"                <div class=\"feature-card\">\n"
"                    <h3>&#x2699; FreeRTOS</h3>\n"
"                    <p>Sistema operacional de tempo real para gerenciamento de tarefas e recursos.</p>\n"
"                </div>\n"
"                <div class=\"feature-card\">\n"
"                    <h3>&#x1F310; LwIP Stack</h3>\n"
"                    <p>Pilha TCP/IP leve e eficiente para conectividade Ethernet.</p>\n"
"                </div>\n"
"                <div class=\"feature-card\">\n"
"                    <h3>&#x1F4E1; Servidor HTTP</h3>\n"
"                    <p>Servidor web integrado para monitoramento e configuracao.</p>\n"
"                </div>\n"
"                <div class=\"feature-card\">\n"
"                    <h3>&#x1F4DA; Codigo Documentado</h3>\n"
"                    <p>Comentarios detalhados e tutoriais para facilitar o aprendizado.</p>\n"
"                </div>\n"
"            </div>\n"
"        </div>\n"
"\n"
"        <div class=\"buttons\">\n"
"            <a href=\"https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP\" \n"
"               class=\"btn btn-github\" target=\"_blank\">\n"
"                &#x1F4BB; GitHub do Projeto\n"
"            </a>\n"
"            <a href=\"https://mcu.tec.br\" class=\"btn btn-site\" target=\"_blank\">\n"
"                &#x1F310; MCU.tec.br\n"
"            </a>\n"
"            <a href=\"https://youtube.com/@mcu_fpga\" class=\"btn btn-youtube\" target=\"_blank\">\n"
"                &#x25B6; Canal YouTube\n"
"            </a>\n"
"        </div>\n"
"\n"
"        <div class=\"footer\">\n"
"            <p>Desenvolvido com &#x2764; por <a href=\"https://github.com/carlosdelfino\">Carlos Delfino</a></p>\n"
"            <p>STM32F429ZI | Cortex-M4 @ 168MHz | Template v1.0.0</p>\n"
"        </div>\n"
"    </div>\n"
"</body>\n"
"</html>\n";

/**
 * @brief Página de erro 404 HTML.
 *
 * Exibida quando o recurso solicitado não é encontrado.
 */
static const char error404Html[] =
"<!DOCTYPE html>\n"
"<html lang=\"pt-BR\">\n"
"<head>\n"
"    <meta charset=\"UTF-8\">\n"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
"    <title>404 - Pagina Nao Encontrada</title>\n"
"    <style>\n"
"        * {\n"
"            margin: 0;\n"
"            padding: 0;\n"
"            box-sizing: border-box;\n"
"        }\n"
"        body {\n"
"            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;\n"
"            background: linear-gradient(135deg, #FF6B35 0%, #8B4513 100%);\n"
"            min-height: 100vh;\n"
"            color: #FFFFFF;\n"
"            display: flex;\n"
"            justify-content: center;\n"
"            align-items: center;\n"
"            padding: 20px;\n"
"        }\n"
"        .container {\n"
"            text-align: center;\n"
"            max-width: 600px;\n"
"        }\n"
"        .error-code {\n"
"            font-size: 8em;\n"
"            font-weight: bold;\n"
"            color: #2E8B57;\n"
"            text-shadow: 4px 4px 8px rgba(0,0,0,0.3);\n"
"            background: rgba(255,255,255,0.9);\n"
"            -webkit-background-clip: text;\n"
"            -webkit-text-fill-color: transparent;\n"
"            background-clip: text;\n"
"        }\n"
"        .error-icon {\n"
"            font-size: 5em;\n"
"            margin: 20px 0;\n"
"        }\n"
"        h1 {\n"
"            font-size: 2em;\n"
"            margin-bottom: 20px;\n"
"            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);\n"
"        }\n"
"        p {\n"
"            font-size: 1.2em;\n"
"            margin-bottom: 30px;\n"
"            opacity: 0.9;\n"
"        }\n"
"        .btn-home {\n"
"            display: inline-block;\n"
"            background: linear-gradient(135deg, #FF6B35, #FF8C42);\n"
"            color: white;\n"
"            text-decoration: none;\n"
"            padding: 15px 40px;\n"
"            border-radius: 30px;\n"
"            font-size: 1.1em;\n"
"            font-weight: bold;\n"
"            border: 3px solid #2E8B57;\n"
"            box-shadow: 0 4px 15px rgba(0,0,0,0.3);\n"
"            transition: all 0.3s ease;\n"
"        }\n"
"        .btn-home:hover {\n"
"            transform: translateY(-3px);\n"
"            box-shadow: 0 6px 20px rgba(0,0,0,0.4);\n"
"            background: linear-gradient(135deg, #FF8C42, #FFA500);\n"
"        }\n"
"        .box {\n"
"            background: rgba(139, 69, 19, 0.3);\n"
"            border: 3px solid #2E8B57;\n"
"            border-radius: 20px;\n"
"            padding: 40px;\n"
"            box-shadow: 0 8px 32px rgba(0,0,0,0.2);\n"
"        }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"    <div class=\"container\">\n"
"        <div class=\"box\">\n"
"            <div class=\"error-code\">404</div>\n"
"            <div class=\"error-icon\">&#x1F50D;</div>\n"
"            <h1>Pagina Nao Encontrada</h1>\n"
"            <p>O recurso que voce esta procurando nao existe neste servidor.</p>\n"
"            <a href=\"/\" class=\"btn-home\">&#x1F3E0; Voltar para Inicio</a>\n"
"        </div>\n"
"    </div>\n"
"</body>\n"
"</html>\n";

/*-----------------------------------------------------------------------------
 * Estrutura do Sistema de Arquivos Embarcado
 *----------------------------------------------------------------------------*/

/**
 * @brief Estrutura para representar um arquivo no sistema embarcado.
 */
typedef struct
{
    const char *name;       /**< Nome do arquivo (ex: "/index.html") */
    const char *data;       /**< Ponteiro para os dados do arquivo */
    uint32_t len;           /**< Tamanho do arquivo em bytes */
    const char *mimeType;   /**< Tipo MIME do arquivo */
} WebFile_t;

/**
 * @brief Lista de arquivos disponíveis no servidor.
 */
static const WebFile_t webFiles[] =
{
    { "",            indexHtml,    sizeof(indexHtml) - 1,    "text/html" },
    { "/",           indexHtml,    sizeof(indexHtml) - 1,    "text/html" },
    { "/index.html", indexHtml,    sizeof(indexHtml) - 1,    "text/html" },
    { "/index.htm",  indexHtml,    sizeof(indexHtml) - 1,    "text/html" },
    { "/404.html",   error404Html, sizeof(error404Html) - 1, "text/html" },
    { NULL,          NULL,         0,                         NULL       }
};

/*-----------------------------------------------------------------------------
 * Implementação das Funções
 *----------------------------------------------------------------------------*/

/**
 * @brief Registra as páginas web no servidor HTTP do LwIP.
 *
 * Esta função configura o sistema de arquivos customizado para
 * que o httpd do LwIP possa servir as páginas embarcadas.
 */
void webpagesRegister(void)
{
    /* 
     * NOTA: O LwIP httpd usa funções de callback para acessar arquivos.
     * As funções fs_open, fs_read e fs_close são implementadas abaixo
     * e linkadas automaticamente pelo linker.
     */
}

/**
 * @brief Busca um arquivo pelo nome.
 *
 * @param name Nome do arquivo a buscar.
 * @return Ponteiro para a estrutura WebFile_t, ou NULL se não encontrado.
 */
static const WebFile_t *findFile(const char *name)
{
    for (int i = 0; webFiles[i].name != NULL; i++)
    {
        if (strcmp(webFiles[i].name, name) == 0)
        {
            return &webFiles[i];
        }
    }
    return NULL;
}

/**
 * @brief Retorna os dados da landing page.
 *
 * @param len Ponteiro para armazenar o tamanho dos dados.
 * @return Ponteiro para os dados HTML.
 */
const char *webpagesGetIndex(uint32_t *len)
{
    if (len != NULL)
    {
        *len = sizeof(indexHtml) - 1;
    }
    return indexHtml;
}

/**
 * @brief Retorna os dados da página 404.
 *
 * @param len Ponteiro para armazenar o tamanho dos dados.
 * @return Ponteiro para os dados HTML.
 */
const char *webpagesGet404(uint32_t *len)
{
    if (len != NULL)
    {
        *len = sizeof(error404Html) - 1;
    }
    return error404Html;
}

/*-----------------------------------------------------------------------------
 * Funções de Interface com o LwIP httpd (fs_*)
 *
 * Estas funções são chamadas pelo httpd do LwIP para acessar arquivos.
 * Implementam a interface definida em lwip/apps/fs.h
 *----------------------------------------------------------------------------*/

#include "lwip/apps/fs.h"

/**
 * @brief Abre um arquivo para leitura.
 *
 * @param file Estrutura de arquivo do LwIP.
 * @param name Nome do arquivo a abrir.
 * @return ERR_OK se sucesso, erro caso contrário.
 */
int fs_open_custom(struct fs_file *file, const char *name)
{
    const WebFile_t *webFile;

    if (file == NULL || name == NULL)
    {
        return 0;
    }

    webFile = findFile(name);

    /* Se não encontrou, usar página 404 */
    if (webFile == NULL)
    {
        webFile = findFile("/404.html");
    }

    if (webFile != NULL)
    {
        file->data = webFile->data;
        file->len = webFile->len;
        file->index = 0;
        file->flags = FS_FILE_FLAGS_HEADER_INCLUDED;
        return 1;
    }

    return 0;
}

/**
 * @brief Fecha um arquivo.
 *
 * @param file Estrutura de arquivo do LwIP.
 */
void fs_close_custom(struct fs_file *file)
{
    (void)file;
    /* Nada a fazer para arquivos em flash */
}

/**
 * @brief Lê dados de um arquivo.
 *
 * @param file Estrutura de arquivo.
 * @param buffer Buffer para armazenar os dados lidos.
 * @param count Número máximo de bytes a ler.
 * @return Número de bytes lidos, ou -1 se fim do arquivo.
 */
int fs_read_custom(struct fs_file *file, char *buffer, int count)
{
    int remaining;
    int toRead;

    if (file == NULL || buffer == NULL)
    {
        return -1;
    }

    remaining = file->len - file->index;
    if (remaining <= 0)
    {
        return -1;
    }

    toRead = (count < remaining) ? count : remaining;
    memcpy(buffer, file->data + file->index, toRead);
    file->index += toRead;

    return toRead;
}

