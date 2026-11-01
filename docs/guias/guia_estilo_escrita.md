# Guia de estilo de escrita científica

Este guia descreve como os textos deste projeto devem soar: títulos, relatórios, protocolos, resenhas, artigos e o TCC. Ele orienta o julgamento de quem escreve e não pretende esgotar o assunto. Instruções da instituição, do periódico ou do evento prevalecem, e ortografia, ABNT e SI seguem as fontes oficiais.

## O tom

O tom é científico, sóbrio, preciso e natural. A formalidade vem da escolha correta dos conceitos e da organização do argumento, não de palavras que soam acadêmicas. Este título serve de referência:

> Síntese dos estudos experimentais do RCSR e das abordagens precursoras

Ele informa a operação (síntese), o objeto (estudos experimentais) e o recorte (RCSR e abordagens precursoras). Não antecipa resultados nem tenta convencer.

A sobriedade aparece quando o número ocupa o lugar do adjetivo e a frase vai direto ao conteúdo:

| Em vez de | Prefira |
|---|---|
| Os resultados do A2 foram promissores. | O A2 superou o A1 em 0,091 de Macro-F1 no teste (IC 95 % de 0,058 a 0,117). |
| Foi realizada uma análise com o objetivo de verificar a existência de possíveis diferenças. | A análise comparou os métodos pelo Macro-F1 em três regiões. |
| Cabe ressaltar que a variante não atingiu o critério. | A variante não atingiu o critério registrado. |

## O leitor

O objetivo é um texto fluido e coerente, que transmita a ideia ao leitor sem linguagem flutuante nem termos aleatórios. Esse padrão vale para um relatório, um TCC ou uma tese, e a qualidade do texto não muda o que a evidência sustenta.

Todo texto tem uma mensagem que cabe em uma ou duas frases, e ela é escrita antes do texto. Quando essa frase não sai, o problema costuma estar no argumento, não na redação. Sem ela, cada revisão melhora uma frase e o conjunto continua sem direção.

Quem escreve conhece demais o próprio trabalho e esquece o que o leitor não sabe. Por isso, cada termo e cada sigla são definidos na primeira ocorrência. Palavras de bastidores, como "portão" e "célula", ficam nos registros de trabalho; no texto para leitor externo, são definidas ou substituídas.

Quem avalia um trabalho lê como pesquisador e também como leitor comum. Espera solidez acadêmica e, ao mesmo tempo, um texto claro, bem acabado e fácil de seguir.

## O parágrafo e a frase

A primeira frase de cada parágrafo diz o que ele afirma. Lidas em sequência, as primeiras frases devem contar o argumento, e uma frase que caberia em qualquer outro trabalho ainda não disse nada. O restante do parágrafo sustenta essa afirmação com evidência e interpretação delimitada.

O corpo do texto usa frases completas, porque são elas que relacionam as informações. O registro condensado fica bem em tabelas e títulos, não no parágrafo:

> **Condensado:** Situação em 30 de outubro de 2026: 53 estudos, cinco fora da rodada principal e um em andamento.
>
> **Contínuo:** Até 30 de outubro de 2026, foram registrados 53 estudos relacionados ao RCSR e às abordagens precursoras. Desses, cinco não avançaram para a etapa principal de experimentação, e um permanece em andamento.

Na frase, o que o leitor já conhece vem no início e a informação nova vem no fim, que é a posição de ênfase. Sujeito e verbo ficam próximos, e dois achados importantes costumam pedir duas frases. Os conectores precisam de função: "entretanto" marca uma oposição real, e "portanto", uma conclusão sustentada. "Nesse contexto" só ajuda quando o contexto está claro.

Cada frase parte de algo que a anterior deixou, e é isso que dá fluidez ao texto. Quando o assunto muda a cada frase, o leitor precisa refazer a ligação sozinho:

> **Salta de assunto:** A classificação da cobertura do solo permite acompanhar a agricultura em grandes áreas, mas os classificadores perdem desempenho fora das regiões de treino. Não foi encontrado estudo que meça essa perda no Brasil.
>
> **Encadeado:** A classificação da cobertura do solo permite acompanhar a agricultura em grandes áreas, mas depende de talhões já rotulados, que faltam em muitas regiões. Por isso, um modelo treinado em uma região precisa funcionar em outras. Nessa transferência, o desempenho costuma cair.

O leitor também precisa saber a que cada expressão se refere. "Isso prejudicou os resultados" obriga a adivinhar; "a redução do número de observações válidas diminuiu o Macro-F1 na região avaliada" não obriga.

## O texto inteiro

Um texto longo se sustenta em um único argumento. Esse argumento liga a pergunta à resposta, as conclusões parciais à evidência e os resultados à literatura. Sem ele, o texto parece desconexo mesmo quando cada parágrafo está bem escrito.

A estrutura fica explícita porque um texto longo raramente é lido de uma vez, e quem chega ao capítulo 5 pode já ter esquecido o capítulo 1. O início de uma seção afirma o que foi feito ou encontrado, e o fim retoma o que ficou estabelecido. Essa sinalização serve ao leitor; quando apenas anuncia o conteúdo ou repete o sumário, sai.

A literatura e os resultados entram como parte do argumento. Uma lista de quem disse o quê não basta: o texto interpreta as fontes, aponta divergências e as usa para justificar a pergunta e situar a contribuição. Com os resultados acontece o mesmo: o texto diz o que eles significam para a pergunta e quais estudos reforçam ou contrariam.

O início e o acabamento pesam. O julgamento do leitor começa no resumo e na introdução e muda devagar. Erros frequentes de digitação, gramática, formatação ou referência também fazem o leitor duvidar da pesquisa, e a partir daí ele lê em busca de falhas.

## As palavras

A palavra certa é a que corresponde à operação executada. Descrever, comparar, avaliar, diagnosticar, sintetizar e validar são coisas diferentes, e "efeito" só cabe quando o desenho isola o fator; nos outros casos, "comparação" ou "associação" dizem o que de fato foi feito.

Repetir um termo técnico é melhor do que variar por elegância, porque a consistência facilita a leitura. Alguns usos já estão assentados neste projeto:
- **Configurações comparadas:** "variante", "variante de referência" e "variante avaliada", nunca "braço".
- **Repetições de treinamento:** *seeds*, porque "sementes" é ambíguo em textos sobre culturas agrícolas.
- **Unidades de contagem:** estudo, experimento, configuração, execução e ajuste de modelo não se confundem; cem ajustes de um experimento não são cem estudos.
- **Domínio:** classificação de culturas, cobertura do solo e uso do solo não são expressões intercambiáveis.
- **Operações:** um verbo não serve a duas operações. O modelo relaciona observações, e o estudo compara modelos; se os dois "comparam", o leitor precisa adivinhar qual é qual.

Linguagem flutuante é a que soa bem e não afirma nada verificável, como "os resultados servem de referência para trabalhos futuros". O teste é perguntar o que o leitor passa a saber com a frase; se a resposta for nada, a frase sai.

Verbos fortes encurtam o texto: "medir", e não "realizar a medição". Qualificadores como "robusto", "superior" e "inovador" só entram quando há critério que os sustente. Nada disso forma uma lista de palavras proibidas: o problema é a falta de função, de definição ou de evidência.

## O tamanho da afirmação

A força da conclusão acompanha a força da evidência. Um resultado em uma região não estabelece desempenho geral, e uma ablação favorável não explica, sozinha, o mecanismo de um método. O texto deixa visível em que nível cada frase está:

| Nível | Exemplo |
|---|---|
| Observação | O menor Macro-F1 foi observado no grupo com menos observações válidas. |
| Interpretação | A diferença é compatível com menor transferência para essa região. |
| Hipótese | A distribuição das classes pode contribuir para a redução observada. |

Resultados negativos são descritos pelo critério e pelo resultado, como em "a variante não atingiu o ganho mínimo registrado". "Fracassou" esconde a diferença entre falha técnica, critério não atendido e evidência inconclusiva. Da mesma forma, diferença não detectada não é equivalência, e "nas regiões e nos períodos avaliados" é mais honesto que "generaliza para o Brasil".

Os números dizem o que medem. Uma diferença é absoluta ou relativa: passar de 0,60 para 0,65 é +0,05 na escala da métrica e 8,3 % em termos relativos. Um intervalo de confiança identifica o que estima, como foi calculado e de onde vem a variação, seja de *seeds*, de amostras ou de regiões. Um resultado também precisa permitir que o leitor reconstrua a comparação: pergunta, métodos, dados e partições, métrica, repetições e critério de decisão, com a indicação do que foi reutilizado.

A honestidade vale também para as fontes. Quem leu só o resumo diz isso, e nenhum dado, causa ou conclusão é presumido para completar o texto.

## Os títulos

Um bom título costuma reunir operação, objeto e recorte, sem que isso vire molde. Ele é tão curto quanto a exatidão permite, não é tão estreito que descreva um só caso nem tão amplo que caiba em qualquer trabalho, e um não especialista consegue entender do que se trata. Datas, contagens, nomes de arquivos e estados de execução ficam nos metadados ou no parágrafo inicial, salvo quando fazem parte do objeto científico, como em "Mudanças na cobertura do solo entre 2010 e 2020".

O título não precisa carregar o trabalho inteiro, porque o resumo e as palavras-chave levam o restante. Ele nomeia o que foi feito: um trabalho que propõe um método não se apresenta como avaliação. Tem um único objeto, porque as classes são saídas do mesmo classificador, e separá-las em grupos sugere assuntos distintos. Compostos de jargão, como "espectrotemporal relacional", ficam para o texto, onde podem ser definidos.

A forma nominal costuma soar melhor do que uma oração presa ao final. Quando o título continua ruim depois de algumas trocas de palavras, o problema está na estrutura, e é ela que precisa mudar.

| Em vez de | Prefira | Por quê |
|---|---|---|
| Situação geral dos testes até agora | Síntese dos resultados experimentais | Retira o tom de acompanhamento informal. |
| Novo método robusto para classificação | Método de classificação baseado em relações temporais | Descreve a proposta sem atribuir qualidade não demonstrada. |
| Estudo sobre algumas dificuldades do modelo | Análise dos erros de classificação entre culturas | Identifica o objeto. |
| Classificação de culturas agrícolas e vegetação | Classificação da cobertura do solo | Trata as classes como um único problema. |
| Para classificar a cobertura do solo entre regiões do Brasil | Classificação da cobertura do solo fora da região de treino | Troca a oração por um nome e diz o que acontece entre as regiões. |

## O resumo e as palavras-chave

As regras de extensão e de formato mudam com o país, a instituição e o departamento, e por isso ficam fora deste guia. A forma de escrever muda pouco: as orientações de Harvard, do MIT e de Oxford pedem, com palavras diferentes, um texto que se sustente sozinho e diga o que foi feito e o que se encontrou.

O resumo responde, nesta ordem, a seis perguntas, e respondê-las antes de redigir mostra o que ainda falta:

1. Qual é o contexto maior, e por que ele importa?
2. Qual é o foco, e qual é a questão central?
3. Qual foi o objetivo?
4. Qual foi a abordagem para responder à questão?
5. Quais dados foram gerados, e quais foram os resultados?
6. O que os resultados implicam para a questão central?

As respostas formam um único parágrafo, sem marcadores, em que cada frase retoma a anterior. Cada resposta costuma caber em uma ou duas frases, e nem todas precisam do mesmo peso. A lacuna entra quando ajuda a formular a questão; uma frase solta sobre o que não foi encontrado interrompe a sequência e fica melhor na introdução.

O texto termina no que os resultados significam, não em uma lista de ressalvas, mas os resultados negativos ficam nele, porque são achados. Nada aparece no resumo sem estar no trabalho, e ele dispensa citações. Quando o corte de palavras deixa as frases telegráficas, ele foi longe demais.

O resumo é onde o leitor de fora mais importa. Um colega de outra área deve conseguir lê-lo e apontar onde travou, e o termo que não for essencial à mensagem sai.

As palavras-chave são os termos que outro pesquisador digitaria para encontrar o trabalho. Como pesam na busca junto com o título e o resumo, os três usam os mesmos termos para os mesmos conceitos, e os mais acessíveis merecem ir para o título quando couberem.

## As convenções da casa

Algumas escolhas são só convenção, e o que importa é mantê-las no documento inteiro:
- **Registro:** terceira pessoa, de preferência com sujeito inanimado, como em "a Tabela 3 mostra". A passiva cabe quando o agente não importa. A primeira pessoa do plural fica para os veículos que a pedem.
- **Tempo verbal:** presente para o que é tomado como geral, e pretérito para o que foi feito e observado.
- **Estrangeirismos:** em itálico, exceto nomes próprios, siglas e nomes de produto.
- **Números:** vírgula decimal, espaço antes de unidades e do sinal de porcentagem, incerteza na forma (75,0 ± 3,5) % e casas decimais coerentes dentro de cada tabela.
- **Métrica:** Macro-F1, sempre com essa grafia.
- **"Cerca de":** só com número redondo; "cerca de 17 mil" funciona, e "cerca de 0,41" não.

## Exemplos de como escrever e de como não escrever

Parte dos pares a seguir saiu de revisões reais dos textos deste projeto, e parte ilustra erros comuns com os mesmos dados. Em cada par, a primeira versão é a que se evita, e a segunda é a que se adota. Esse padrão permanece até que o autor aponte um erro e peça mudança.

### A abertura

A abertura afirma o que foi feito ou encontrado. Anunciar o capítulo adia a informação, repete o sumário e produz uma frase que caberia em qualquer trabalho.

> **Em vez de:** Este capítulo reúne o necessário para reconstruir as comparações do resultado central: a origem dos dados, o que cada observação contém, como os talhões foram divididos, o que se mede e qual regra decide cada comparação.
>
> **Prefira:** O RCSR foi comparado com as demais variantes em 4.899 talhões de Mato Grosso, do Paraná e da Bahia, observados pelo Sentinel-2 em uma safra por região. Cada modelo foi treinado em parte desses talhões e avaliado em uma área fora do treino, pelo Macro-F1, com uma regra de decisão registrada antes do teste.

A seção e o parágrafo seguem a mesma lógica:

> **Em vez de:** A seguir, são apresentadas as características das séries temporais obtidas.
>
> **Prefira:** As séries resultantes são irregulares e diferem entre talhões.

### As frases

Uma frase de transição ou uma frase que só aponta para uma tabela pode sair sem perda. A tabela entra como citação na frase que ela sustenta.

> **Em vez de:** Cada conjunto cobre áreas e períodos próprios. O Brazil-Crop fornece 1.769 talhões em Campo Verde, em Mato Grosso.
>
> **Prefira:** O Brazil-Crop fornece 1.769 talhões em Campo Verde, em Mato Grosso.

> **Em vez de:** Mato Grosso, Paraná e Bahia identificam essas áreas. A Tabela 3.1 resume a origem dos talhões.
>
> **Prefira:** Neste texto, Mato Grosso, Paraná e Bahia identificam essas áreas, e não os estados inteiros (Tabela 3.1).

Fórmulas prontas alongam a frase sem acrescentar informação, e o registro de anotação fica nas tabelas.

> **Em vez de:** Trata-se de uma referência heurística, e não de uma garantia de poder estatístico.
>
> **Prefira:** O MDE é uma referência heurística, e não uma garantia de poder estatístico.

> **Em vez de:** Máscara: SCL 4/5/6; sem filtro de nuvem por cena.
>
> **Prefira:** Um pixel só é aceito quando a classificação de cena do próprio produto (SCL) o marca como vegetação, área sem vegetação ou água. Não houve filtro de nuvem por cena: uma cena encoberta aparece como observação ausente nos talhões afetados.

### O encadeamento

Cada frase parte do que a anterior deixou. Na primeira versão abaixo, cada frase começa em um assunto novo e o leitor precisa descobrir como elas se ligam.

> **Em vez de:** O treino da região-fonte estima os parâmetros e a normalização, e a validação da fonte escolhe a época. Rótulos da região-alvo não participam dessas etapas. A análise de desenvolvimento usa os talhões de validação do alvo, e a análise de teste aplica os mesmos modelos aos talhões de teste do alvo.
>
> **Prefira:** Cada ajuste de modelo corresponde a uma variante, uma combinação e uma *seed*. Nele, o treino da fonte estima os parâmetros e a normalização, e a validação da fonte escolhe a época, sem uso de rótulos do alvo. Os mesmos modelos são então avaliados duas vezes no alvo: nos talhões de validação, na análise de desenvolvimento, e nos talhões de teste, na análise final.

Um conector só entra quando a relação existe. No par seguinte, a contaminação residual não decorre da frase anterior, e "portanto" promete uma conclusão que não há.

> **Em vez de:** Uma data é válida para o talhão quando ao menos um pixel foi aceito e os doze canais têm valor definido. Uma observação válida, portanto, pode conter contaminação residual.
>
> **Prefira:** Não houve filtro de nuvem por cena: uma cena encoberta aparece como observação ausente nos talhões afetados. A máscara, porém, não exclui contaminação residual em uma observação válida.

A frase também diz a que se refere e qual é a consequência.

> **Em vez de:** Isso limita a interpretação dos resultados.
>
> **Prefira:** Cada região foi observada em uma única safra. Uma diferença entre regiões pode refletir o local, o ano ou os dois, e os dados não permitem separar esses fatores.

> **Em vez de:** A escolha do vocabulário afeta a métrica.
>
> **Prefira:** Como o vocabulário é fixo, uma classe ausente na região avaliada entra na média com valor zero e limita o Macro-F1 possível.

### As palavras

Termos de bastidores são substituídos por aquilo que designam, e um termo necessário é definido no primeiro uso.

> **Em vez de:** As oito células atenderam ao suporte mínimo.
>
> **Prefira:** O registro exigia ao menos 20 talhões e duas classes presentes para avaliar uma combinação no teste, condição atendida pelas oito.

> **Em vez de:** O vocabulário de cada estação resultou de uma regra de suporte aplicada às contagens de desenvolvimento.
>
> **Prefira:** O conjunto de classes de cada estação, aqui chamado de vocabulário, foi definido por contagens de talhões, sem uso de desempenho de modelos.

Um conceito mantém o mesmo termo do começo ao fim.

> **Em vez de:** As parcelas vêm de dois conjuntos públicos, e cada campo corresponde a um exemplo.
>
> **Prefira:** Os talhões vêm de dois conjuntos públicos, e cada talhão corresponde a um exemplo.

### Os números

O número ocupa o lugar do adjetivo e precisa medir exatamente o que a frase afirma.

> **Em vez de:** A soja domina o verão nas três regiões.
>
> **Prefira:** Em cada região, a soja reúne de 73 % a 80 % dos talhões de verão usados em treino e validação.

| Em vez de | Prefira | Por quê |
|---|---|---|
| Nos 1.456 talhões de teste | Nos 1.027 talhões de teste | 1.456 é o tamanho da partição, e 1.027 é o número de talhões avaliados. |
| parte já usada em estudo anterior | 382 já usados em estudo anterior | "Parte" esconde que são 37 % dos talhões avaliados. |
| de mesma capacidade | de capacidade equivalente | A variante de referência tem 26 parâmetros a mais. |

### O tamanho da afirmação

A frase não diz mais do que o desenho do estudo permite. Causa, prioridade e alcance são os excessos mais frequentes.

> **Em vez de:** O ganho decorre da agregação por faixas de dias.
>
> **Prefira:** Nenhuma variante foi treinada sem faixas, e por isso os dados não permitem atribuir o ganho a elas.

> **Em vez de:** O RCSR é o primeiro método a agregar relações por faixas de dias reais.
>
> **Prefira:** A busca, que não foi exaustiva, não encontrou precedente para a agregação por faixas de dias reais.

> **Em vez de:** Os resultados abrem caminho para a aplicação do RCSR em todo o Brasil.
>
> **Prefira:** Como cada região tem uma só safra, os resultados não sustentam generalização para outros anos.

> **Em vez de:** O intervalo de confiança mostra que o ganho se mantém em outras regiões.
>
> **Prefira:** O intervalo incorpora a variação entre blocos e entre inicializações. Ele não incorpora a variação entre regiões nem entre anos e, por isso, não estima o desempenho em uma região nova.

> **Em vez de:** O RCSR fracassou diante do Random Forest.
>
> **Prefira:** O RCSR não demonstrou superioridade sobre um Random Forest, por uma perda de 0,129 na Bahia, no inverno.

Pequenas palavras também mudam o tamanho da afirmação.

| Em vez de | Prefira | Por quê |
|---|---|---|
| sem datas | sem datas de calendário | O modelo usa intervalos em dias entre as observações. |
| registrada antes dos treinos | registrada antes do teste | A regra completa foi registrada depois dos primeiros ajustes e antes da abertura do teste. |

### Um parágrafo de referência

Este parágrafo reúne o padrão: a primeira frase diz o que ele afirma, as seguintes sustentam essa afirmação com fatos, cada uma retoma a anterior, e os números ocupam o lugar dos adjetivos.

> Os blocos separam papéis, mas não garantem independência espacial. O protocolo original das partições excluía os talhões que cruzam a borda de um bloco ou ficam a menos de 30 m dela, o que asseguraria ao menos 60 m entre talhões de blocos diferentes. As comparações deste trabalho não aplicam essa exclusão: 832 dos 3.872 talhões de desenvolvimento e 255 dos 1.027 de teste estão nessa situação.

## A última leitura

A revisão final faz poucas perguntas. A abertura afirma algo ou só anuncia o que vem? As primeiras frases contam o argumento? Cada frase retoma a anterior? Cada conceito tem um único termo? O leitor consegue reconstruir cada comparação? Alguma frase atribui causa, novidade ou generalidade sem sustentação? Dá para retirar palavras sem perder informação?

As ideias deste guia vêm sobretudo de Gopen e Swan (*The Science of Scientific Writing*, 1990), da revisão de Golding, Sharmini e Lazarovitch sobre o trabalho dos examinadores de teses (*What examiners do*, 2014), do CommKit do MIT Communication Lab, do Writing Center e dos guias de tese de Harvard, de orientações de escrita da Universidade de Oxford, do formulário *Abstract: Constructing the Paragraph* e do *Manual de Redação da Presidência da República*. O guia é uma adaptação aos projetos do autor, não um resumo dessas fontes, e parte delas foi consultada apenas de forma indireta.
