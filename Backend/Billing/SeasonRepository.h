#pragma once
#ifndef SEASONREPOSITORY_H
#define SEASONREPOSITORY_H

#include <string>
#include <vector>
#include "BillCalc3.h"

class FileSeasonRepository : public SeasonSource
{
private:
    std::string filePath;

public:
    explicit FileSeasonRepository(const std::string& path = "Data/Seasons.txt");

    std::vector<SeasonRule> getSeasonRules() const override;
    bool addSeasonRule(const SeasonRule& rule) const;
    bool saveAllSeasonRules(const std::vector<SeasonRule>& rules) const;
};

#endif