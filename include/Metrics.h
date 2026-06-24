#pragma once

class Metrics
{
public:

    static double accuracy(
        int correct,
        int total);

    static double precision(
        int tp,
        int fp);

    static double recall(
        int tp,
        int fn);

    static double f1Score(
        int tp,
        int fp,
        int fn);
};