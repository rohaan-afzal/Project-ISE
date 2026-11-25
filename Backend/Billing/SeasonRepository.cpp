#include "SeasonRepository.h"
#include "RecordFormat3.h"
#include <fstream>

FileSeasonRepository::FileSeasonRepository(const std::string& path)
    : filePath(path)
{
}

std::vector<SeasonRule> FileSeasonRepository::getSeasonRules() const
{
    std::vector<SeasonRule> rules;
    std::ifstream file(filePath.c_str());
    if (!file.is_open())
        return rules;

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        std::vector<std::string> fields;
        if (!RecordFormat::splitLine(line, fields) || fields.size() != 4)
            continue;

        SeasonRule rule;
        rule.startDate = fields[0];
        rule.endDate = fields[1];

        if (!RecordFormat::toDouble(fields[2], rule.percentage))
            continue;

        int activeFlag = 0;
        if (RecordFormat::toInt(fields[3], activeFlag))
            rule.isActive = (activeFlag != 0);
        else if (fields[3] == "true" || fields[3] == "True" || fields[3] == "ACTIVE")
            rule.isActive = true;
        else if (fields[3] == "false" || fields[3] == "False" || fields[3] == "INACTIVE")
            rule.isActive = false;
        else
            continue;

        rules.push_back(rule);
    }

    return rules;
}

bool FileSeasonRepository::addSeasonRule(const SeasonRule& rule) const
{
    if (!SeasonalDiscountService::isValidDate(rule.startDate) ||
        !SeasonalDiscountService::isValidDate(rule.endDate))
        return false;
    if (rule.percentage < 0.0 || rule.percentage > 100.0)
        return false;

    std::ofstream file(filePath.c_str(), std::ios::app);
    if (!file.is_open())
        return false;

    std::vector<std::string> fields = {
        rule.startDate,
        rule.endDate,
        RecordFormat::formatNumber(rule.percentage),
        rule.isActive ? "1" : "0"
    };

    file << RecordFormat::joinFields(fields) << '\n';
    file.flush();
    return !file.fail();
}

bool FileSeasonRepository::saveAllSeasonRules(const std::vector<SeasonRule>& rules) const
{
    std::ofstream file(filePath.c_str(), std::ios::trunc);
    if (!file.is_open())
        return false;

    for (const auto& rule : rules)
    {
        std::vector<std::string> fields = {
            rule.startDate,
            rule.endDate,
            RecordFormat::formatNumber(rule.percentage),
            rule.isActive ? "1" : "0"
        };
        file << RecordFormat::joinFields(fields) << '\n';
    }

    file.flush();
    return !file.fail();
}       