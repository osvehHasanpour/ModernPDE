#include "Metrics.h"

double Metrics::accuracy(
    int correct,
    int total)
{
    if(total == 0)
        return 0.0;

    return
        static_cast<double>(correct)
        /
        total;
}

double Metrics::precision(
    int tp,
    int fp)
{
    if(tp + fp == 0)
        return 0.0;

    return
        static_cast<double>(tp)
        /
        (tp + fp);
}

double Metrics::recall(
    int tp,
    int fn)
{
    if(tp + fn == 0)
        return 0.0;

    return
        static_cast<double>(tp)
        /
        (tp + fn);
}

double Metrics::f1Score(
    int tp,
    int fp,
    int fn)
{
    double p =
        precision(tp,fp);

    double r =
        recall(tp,fn);

    if(p + r == 0.0)
        return 0.0;

    return
        2.0 *
        p *
        r /
        (p + r);
}