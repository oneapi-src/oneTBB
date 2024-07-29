# oneTBB Design Documents / RFCs

**oneTBB has not yet started to use the RFC process. This branch is a 
work-in-progress.**

The purpose of the RFC process is to communicate the intent to make
library-wide changes, get feedback prior to the actual implementation,
increase the transparency on why and how decisions are made, and improve
the alignment between different teams involved in oneTBB development. 


This branch contains design documents (RFCs) that are approved for
implementation in oneTBB.

## Document Style

The design documents are stored in the `rfcs` directory.

- Each RFC is stored in a separate subdirectory
  `rfcs/<YYYYMMDD>-descriptive-but-short-name`.

  - There must be a `README.md` file that contains the main RFC itself.

  - The directory can contain other supporting files, such as images,
    tex formulas, and sub-proposals / sub-RFCs.

- The RFC is written in markdown. The width of the text should be limited by
  80 symbols, unless there is a need to violate this rule, e.g. because of
  long links or wide tables.

- The document structure should follow the [RFC template](rfcs/template.md).

  - It is also recommended to read through existing RFCs to better understand
    the general writing style and required elements.

## RFC Ratification Process

TBD
