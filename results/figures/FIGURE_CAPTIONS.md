# Figure captions and source data

All figures were generated from the project CSV outputs supplied for review.

1. figure_1_analyst_revision_share.png
   Mean nanoseconds per attempted operation at each revision share, with sample standard deviation across five repetitions. Source: results/analyst.csv. These runs are very short and show coarse timing increments, so the plot is descriptive rather than strong evidence of a performance winner.

2. figure_2_simulator_wait_by_severity.png
   Mean, median and 95th-percentile waiting times by severity for the stepwise30 rows. Source: results/simulator.csv. The supplied CSV reports the same wait summaries for none, linear and stepwise30 policies; this figure is a representative view, not a comparison proving the policies equivalent in general.

3. figure_3_aging_updates.png
   Aging updates recorded by policy. Source: results/simulator.csv. The CSV repeats the global stepwise update count on each severity row; the count is taken once per policy, not summed across severities.

4. figure_4_adversary_comparisons.png
   Comparison counts for the sift_up and extract_shuffled scenarios. Source: results/raw/adversary_results.csv.

5. figure_5_lazy_heap_compaction.png
   Live, physical and peak physical entry counts for the revisions scenario. Source: results/raw/adversary_results.csv. The observed peak physical count is 10,000 for 5,000 live entries, consistent with the intended two-times-live bound in this recorded scenario.
