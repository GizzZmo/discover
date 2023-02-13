/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <Category/CategoriesReader.h>
#include <Category/Category.h>
#include <QList>
#include <QtTest>

class CategoriesTest : public QObject
{
    Q_OBJECT
public:
    CategoriesTest()
    {
    }

    QVector<Category *> populateCategories()
    {
        const QVector<QString> categoryFiles = {
            QFINDTESTDATA("../backends/PackageKitBackend/packagekit-backend-categories.xml"),
            QFINDTESTDATA("../backends/FlatpakBackend/flatpak-backend-categories.xml"),
            QFINDTESTDATA("../backends/DummyBackend/dummy-backend-categories.xml"),
        };

        QVector<Category *> ret;
        CategoriesReader reader;
        for (const QString &name : categoryFiles) {
            qDebug() << "doing..." << name;
            const QVector<Category *> cats = reader.loadCategoriesPath(name);

            if (ret.isEmpty()) {
                ret = cats;
            } else {
                for (Category *c : cats)
                    Category::addSubcategory(ret, c);
            }
        }
        std::sort(ret.begin(), ret.end(), Category::categoryLessThan);
        return ret;
    }

private Q_SLOTS:
    void testReadCategories()
    {
        auto categories = populateCategories();
        QVERIFY(!categories.isEmpty());

        for (Category *c : categories) {
            if (c->name() != "Dummy Category")
                continue;

            auto filter = c->filter();
            QVERIFY(filter.type == CategoryFilter::CategoryNameFilter);
            QVERIFY(std::get<QString>(filter.value) == "dummy");
        }
    }
};

QTEST_MAIN(CategoriesTest)

#include "CategoriesTest.moc"
