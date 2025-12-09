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
#include <stdio.h>

#include "rest_dispatcher.h"
#include "lwip/apps/fs.h"
#include "fsdata.h"

/*-----------------------------------------------------------------------------
 * Implementação das Funções
 *----------------------------------------------------------------------------*/

/**
 * @brief Busca um arquivo no sistema gerado pelo makefsdata (fsdata.h).
 *
 * Percorre a lista encadeada de estruturas fsdata_file iniciando em FS_ROOT.
 *
 * @param name Nome do arquivo a buscar (ex.: "/index.html" ou "/assets/...").
 * @return Ponteiro para struct fsdata_file ou NULL se não encontrado.
 */
static const struct fsdata_file *findFile(const char *name)
{
    const struct fsdata_file *file;

    if (name == NULL)
    {
        return NULL;
    }

    printf("[HTTP] Busca arquivo que tem o nome %s\n\r", name);

    for (file = FS_ROOT; file != NULL; file = file->next)
    {
        const unsigned char *fname = file->name;

        if (fname != NULL)
        {
            if (strcmp((const char *)fname, name) == 0)
            {
                return file;
            }
        }
    }

    return NULL;
}

/**
 * @brief Retorna os dados da landing page.
 *
 * Usa o arquivo "/index.html" gerado no fsdata.h.
 *
 * @param len Ponteiro para armazenar o tamanho dos dados.
 * @return Ponteiro para os dados HTML ou NULL se não encontrado.
 */
const char *webpagesGetIndex(uint32_t *len)
{
    const struct fsdata_file *file;

    file = findFile("/index.html");
    if (file == NULL)
    {
        file = findFile("/");
    }

    if (file == NULL)
    {
        return NULL;
    }

    if (len != NULL)
    {
        *len = (uint32_t)file->len;
    }

    return (const char *)file->data;
}

/**
 * @brief Retorna os dados da página 404.
 *
 * Usa o arquivo "/404.html" gerado no fsdata.h.
 *
 * @param len Ponteiro para armazenar o tamanho dos dados.
 * @return Ponteiro para os dados HTML ou NULL se não encontrado.
 */
const char *webpagesGet404(uint32_t *len)
{
    const struct fsdata_file *file;

    file = findFile("/404.html");
    if (file == NULL)
    {
        return NULL;
    }

    if (len != NULL)
    {
        *len = (uint32_t)file->len;
    }

    return (const char *)file->data;
}

/*-----------------------------------------------------------------------------
 * Funções de Interface com o LwIP httpd (fs_*)
 *
 * Estas funções são chamadas pelo httpd do LwIP para acessar arquivos.
 * Implementam a interface definida em lwip/apps/fs.h
 *----------------------------------------------------------------------------*/

/**
 * @brief Abre um arquivo para leitura.
 *
 * @param file Estrutura de arquivo do LwIP.
 * @param name Nome do arquivo a abrir.
 * @return ERR_OK se sucesso, erro caso contrário.
 */
int fs_open_custom(struct fs_file *file, const char *name)
{
    const struct fsdata_file *fsdataFile;

    if (file == NULL || name == NULL)
    {
        return 0;
    }

    /* Primeiro, tenta despachar via REST para rotas com prefixo /api/v */
    if (strncmp(name, "/api/v", 6) == 0)
    {
        if (restDispatch(name, "GET", NULL, 0, file) != 0)
        {
            return 1;
        }
    }

    /* Em seguida, busca no sistema de arquivos gerado (fsdata.h) */
    fsdataFile = findFile(name);

    /* Se não encontrou, tenta página inicial ou 404 como fallback */
    if (fsdataFile == NULL)
    {
        if ((strcmp(name, "/") == 0) || (strcmp(name, "") == 0))
        {
            fsdataFile = findFile("/index.html");
        }
        else
        {
            fsdataFile = findFile("/404.html");
        }
    }

    if (fsdataFile != NULL)
    {
        file->data = (const char *)fsdataFile->data;
        file->len = fsdataFile->len;
        file->index = 0;
        /* Arquivos possuem cabeçalho HTTP embutido gerado pelo makefsdata
           (incluindo Content-Type correto para .js/.map via CONTENT_TYPE_MAP),
           portanto informamos ao httpd que o header já está incluso. */
        file->flags = fsdataFile->flags;
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

