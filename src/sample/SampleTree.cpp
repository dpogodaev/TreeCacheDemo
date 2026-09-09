#include "sample/SampleTree.h"

#include <QString>

#include "domain/Id.h"

std::vector<TreeDbElement> buildSampleTree()
{
    std::vector<TreeDbElement> elements;
    Id next = 1;

    const auto addElement = [&next, &elements](const Id parentId, const char* text)
    {
        const Id id = next++;
        elements.push_back(TreeDbElement{
            .id = id,
            .parentId = parentId,
            .text = QString::fromUtf8(text),
            .isDeleted = false
        });
        return id;
    };

    const Id company = addElement(RootId, "Company");

    const Id dept1 = addElement(company, "Department-1");
    const Id unit1A = addElement(dept1, "Unit-1A");
    const Id team1A1 = addElement(unit1A, "Team-1A1");
    addElement(team1A1, "Bob");
    addElement(team1A1, "Mike");
    const Id team1A2 = addElement(unit1A, "Team-1A2");
    addElement(team1A2, "Ivan");
    addElement(team1A2, "Jim");
    const Id unit1B = addElement(dept1, "Unit-1B");
    addElement(unit1B, "Team-1B1");
    const Id team1B2 = addElement(unit1B, "Team-1B2");
    addElement(team1B2, "John");
    addElement(team1B2, "Den");

    const Id dept2 = addElement(company, "Department-2");
    const Id unit2A = addElement(dept2, "Unit-2A");
    const Id team2A1 = addElement(unit2A, "Team-2A1");
    addElement(team2A1, "Alice");
    addElement(unit2A, "Team-2A2");
    const Id unit2B = addElement(dept2, "Unit-2B");
    addElement(unit2B, "Team-2B1");
    const Id team2B2 = addElement(unit2B, "Team-2B2");
    addElement(team2B2, "Oleg");
    addElement(team2B2, "Frank");

    return elements;
}
