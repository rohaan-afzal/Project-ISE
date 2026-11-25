#pragma once

#include <QPushButton>

class DescriptionButton final : public QPushButton {
public:
    explicit DescriptionButton(QWidget* parent = nullptr)
        : QPushButton("Description", parent) {}
};
