#pragma once

class color3
{
public:
    color3()
    {
        c_[0] = 0.0;
        c_[1] = 0.0;
        c_[2] = 0.0;
    }

    color3(double y)
    {
        c_[0] = y;
        c_[1] = y;
        c_[2] = y;
    }

    color3(double r, double g, double b)
    {
        c_[0] = r;
        c_[1] = g;
        c_[2] = b;
    }

    operator const double *() { return c_; }

    color3
    bleach(double factor)
    {
        return color3(1.0 - (1.0 - c_[0]) * factor, 1.0 - (1.0 - c_[1]) * factor,
                      1.0 - (1.0 - c_[2]) * factor);
    }

    void
    rgb(double r, double g, double b)
    {
        c_[0] = r;
        c_[1] = g;
        c_[2] = b;
    }

private:
    double c_[3];
};
