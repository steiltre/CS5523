import os
from bs4 import BeautifulSoup
import numpy as np
import csv

import porter_stemmer

#
# @brief Preprocess Reuters 21578 for clustering
#
# @param dir Directory containing Reuters dataset
#
def preprocess(dir, stoplist_filename):
    # Create Porter stemmer object for stemming words
    p = porter_stemmer.PorterStemmer()

    with open(stoplist_filename) as file:
        stoplist = file.read().split()

    topic_counts = dict()

    for dirname, subdirnames, filenames in os.walk(dir):
        #filenames = ['reut2-000.sgm']      # DON'T FORGET TO REMOVE THIS WHEN USING ALL ARTICLES!!!!!!!!!!!!!!!!!!!!!
        for filename in filenames:
            if ( os.path.splitext(filename)[-1].lower() == '.sgm' ):   # Only pick out sgm files
                with open(dir + '/' +  filename) as file:
                    soup = BeautifulSoup(file, 'html.parser')
                    articles = soup.find_all('reuters')
                    for article in articles:
                        if ( (len(article.topics.contents) == 1) & (article.body is not None) ):     # Only count articles with a single topic
                            topic_counts[article.topics.text] = topic_counts.get(article.topics.text, 0) + 1

    sorted_topics = sorted(topic_counts, key=topic_counts.get, reverse=True)
    popular_topics = sorted_topics[0:20]      # Pick out top 20 occurring topics

    filtered_articles = dict()
    article_tokens = dict()

    # Walk back through files and add files with appropriate topics to dictionary
    for dirname, subdirnames, filenames in os.walk(dir):
        #filenames = ['reut2-000.sgm']      # DON'T FORGET TO REMOVE THIS WHEN USING ALL ARTICLES!!!!!!!!!!!!!!!!!!!!!
        for filename in filenames:
            if ( os.path.splitext(filename)[-1].lower() == '.sgm' ):
                with open(dir + '/' + filename) as file:
                    soup = BeautifulSoup(file, 'html.parser')
                    articles = soup.find_all('reuters')
                    for article in articles:
                        if (article.body is not None):       # Don't consider articles with no body
                            if ( (len(article.topics.contents) == 1) & (article.topics.text in popular_topics) ):
                                text = replace_non_alphanumer(ascii_only(article.body.text).lower())
                                text_set = set(text.split(' '))
                                stemmed_text = stem_text(text_set, stoplist, p)
                                add_tokens(stemmed_text, article_tokens)

    # Create new dictionary of frequent tokens
    frequent_tokens = dict()
    for word in article_tokens:
        if article_tokens[word] >= 5:
            frequent_tokens[word] = article_tokens[word]

    # Sort tokens alphabetically and change values to be index of words
    alphabetic_tokens = sorted(frequent_tokens)
    i = 0
    for word in alphabetic_tokens:
        frequent_tokens[word] = i
        i += 1

    #Open files for writing
    norm_freq_file = open('freq.csv', 'w')
    norm_sqrt_freq_file = open('sqrtfreq.csv', 'w')
    norm_log_freq_file = open('log2freq.csv', 'w')

    class_file = open('reuters21578.class', 'w')
    label_file = open('reuters21578.clabel', 'w')

    write_label_file(alphabetic_tokens, label_file)

    # Walk through files again to determine number of times each token is used in articles
    for dirname, subdirnames, filenames in os.walk(dir):
        #filenames = ['reut2-000.sgm']       # DON'T FORGET TO REMOVE THIS WHEN USING ALL ARTICLES!!!!!!!!!!!!!!!!!!!!!
        for filename in filenames:
            if ( os.path.splitext(filename)[-1].lower() == '.sgm' ):     # Only pick out .sgm files
                with open(dir + '/' + filename) as file:
                    soup = BeautifulSoup(file, 'html.parser')
                    articles = soup.find_all('reuters')
                    for article in articles:
                        if (article.body is not None):
                            if ( (len(article.topics.contents) == 1) & (article.topics.text in popular_topics) ):
                                text = replace_non_alphanumer(ascii_only(article.body.text).lower())
                                [freq, ind] = sparse_article(text, frequent_tokens, stoplist, p)
                                norm_freq = normalize(freq)
                                norm_sqrt_freq = normalize(1 + np.sqrt(freq))
                                norm_log_freq = normalize(1 + np.log2(freq))

                                write_freq_file(norm_freq, ind, article['newid'], norm_freq_file)
                                write_freq_file(norm_sqrt_freq, ind, article['newid'], norm_sqrt_freq_file)
                                write_freq_file(norm_log_freq, ind, article['newid'], norm_log_freq_file)

                                write_article_class(article.topics.text, article['newid'], class_file)

    norm_freq_file.close()
    norm_sqrt_freq_file.close()
    norm_log_freq_file.close()
    class_file.close()
    label_file.close()

    return

#
# @brief Remove all non-ascii characters
#
# @param str String
#
# @return String without non-ascii characters
#
def ascii_only(str):
    return ''.join(i for i in str if ord(i)<128)

#
# @brief Remove all non-alphanumeric characters
#
# @param str String
#
# @return String without alphanumerics
#
def replace_non_alphanumer(str):
    return ''.join([ char if char.isalnum() else ' ' for char in str ])

#
# @brief Add tokens to dictionary of words and counts
#
# @param text_set Set of words to add to dictionary
# @param dict Dictionary to add words to
#
def add_tokens(text_set, dict):
    for word in text_set:
        dict[word] = dict.get(word, 0) + 1

#
# @brief Turn set of words into set of stemmed words
#
# @param text_set Set of words to be stemmed
# @param stoplist List of words to ignore
# @param p Porter stemmer object
#
# @return Set of stemmed words
#
def stem_text(text_set, stoplist, p):
    stemmed_text = set()
    for word in text_set:
        if ( (not word.isnumeric()) & (not word in stoplist) & (word != u'') ):         # Remove unwanted words
            stemmed_text.add(p.stem(word, 0, len(word)-1))

    return stemmed_text

#
# @brief Give a sparse representation of an article as the nonzero frequencies of tokens
#
# @param article Article text to represent
# @param tokens Dictionary of tokens with associated indices
# @param stoplist List of words to ignore
# @param p Porter stemmer object
#
def sparse_article(article, tokens, stoplist, p):
    article = article.split(' ')

    article_tokens = []
    freq = []
    ind = []
    # Create tokens for all words in article
    for word in article:
        if ( (not word.isnumeric()) & (not word in stoplist) & (word != u'') ):
            article_tokens.append(p.stem(word, 0, len(word)-1))

    for token in sorted(article_tokens):
        index = tokens.get(token, -1)
        if (index == -1):
            continue
        elif (len(ind) == 0):
            ind.append(index)
            freq.append(1)
        elif (ind[ len(ind)-1 ] == index):
            freq[ len(freq)-1 ] += 1
        else:
            ind.append(index)
            freq.append(1)

    return [freq,ind]

#
# @brief Normalize a vector
#
# @param vec Vector of nonzero entries
#
# @return Normalized vector
#
def normalize(vec):
    sum_squares = 0.0
    for entry in vec:
        sum_squares += entry**2

    return np.array(vec) / sum_squares

#
# @brief Write (normalized) frequencies to .csv file
#
# @param freq Array of frequencies
# @param ind Array of indices for columns corresponding to frequencies
# @param article_ID ID of current article
# @param file File to write to
#
def write_freq_file(freq, ind, article_ID, file):
    for i in range(len(freq)):
        file.write(str(article_ID) + ', ' + str(ind[i]) + ', ' + str(freq[i]) + '\n')

#
# @brief Write label file
#
# @param labels Ordered list of labels
# @param file File to write to
#
def write_label_file(labels, file):
    for label in labels:
        file.write( str(label) + '\n' )

    return

#
# @brief Write article topic to file
#
# @param topic Article topic
# @param article_ID ID of article
# @param file File to write to
#
def write_article_class(topic, article_ID, file):
    file.write( str(article_ID) + ', ' + str(topic) + '\n' )

    return

if __name__ == '__main__':
    preprocess("./reuters21578", "./reuters21578/stoplist.txt")

