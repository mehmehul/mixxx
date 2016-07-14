#include <QRegExp>
#include <QString>

#include "stringhelper.h"

StringHelper::StringHelper() {

}

QChar StringHelper::getFirstCharForGrouping(const QString& text) {
    if (text.size() <= 0) {
        return QChar();
    }
    
    QChar c = text.at(0);
    if (!c.isLetter()) {
        return c;
    }
    c = c.toUpper();
    QString letter(c);
    QString limmit("Z");
    if (QString::localeAwareCompare(limmit, letter) < 0) {
        // The letter is above z so we must not change it this is due to 
        // Chinese letters or Finnish sorting. In Finnish the correct sorting
        // is a-z, å, ä, ö and the user will expect these letters at the
        // end and not like this a, ä, å, b-o, ö, p-z
        return c;
    }
    
    // This removes the accents of the characters
    QString s1 = letter.normalized(QString::NormalizationForm_KD);
    s1.remove(QRegExp("[^A-Z]"));
    
    if (s1.size() > 0) {
        c = s1.at(0).toUpper();
    }
    return c;
}

