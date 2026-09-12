#pragma once

#include <QString>
#include <QUrl>

/**
 * @brief Encapsulates creator identity, social media handles, and project links.
 */
struct CreatorProfile {
    QString name{QStringLiteral("Suryansh Swarn")};
    QString handle{QStringLiteral("@SuryanshSwarn09")};
    QString role{QStringLiteral("Creator & Lead Software Architect")};
    QString bio{QStringLiteral("Engineered Mem-Map in modern C++20 and Qt 6 for high-performance disk space analysis and storage forensics.")};
    
    QString githubProfileUrl{QStringLiteral("https://github.com/SuryanshSwarn09")};
    QString githubRepoUrl{QStringLiteral("https://github.com/SuryanshSwarn09/Mem-Map")};
    QString email{QStringLiteral("suryanshswarn@gmail.com")};
    QString linkedinUrl{QStringLiteral("https://www.linkedin.com/in/suryanshswarn")};
    QString twitterUrl{QStringLiteral("https://x.com/suryanshswarn09")};

    [[nodiscard]] bool isValid() const {
        return !name.isEmpty() && !githubProfileUrl.isEmpty() && !githubRepoUrl.isEmpty();
    }

    [[nodiscard]] QUrl getEmailUrl() const {
        return QUrl(QStringLiteral("mailto:") + email);
    }

    static CreatorProfile defaultProfile() {
        return CreatorProfile();
    }
};
