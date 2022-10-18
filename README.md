<a name="readme-top"></a>

<!-- project shields -->
![status: active](https://github.com/GIScience/badges/raw/master/status/active.svg)
[![Issues][issues-shield]][license-url]
[![Forks][forks-shield]][forks-url]
[![Stars][stars-shield]][stars-url]
[![MIT License][license-shield]][license-url]

<!-- project header -->
<br/>
<div align="center">
  <h3 align="center">'Avoidable' ED attendances</h3>
  <p align="center">
    The Economics and Strategic Analysis Team, working under the Chief Data and Analytics Officer (CDAO) at NHS England have developed a small package which implements the University of Sheffield's School of Health and Related Research's (ScHARR) definition of 'avoidable' Emergency Department attendances (Mason et al, 2017) with reference to the NHS Digital (2020) adaptation using SNOMED/HES coding. Whilst ECDS superseeds HES, functionality for HES codes is provided for completeness.
    <br/>
    <a href="https://github.com/nhsengland/ESA_Avoidable_ED_Attendances/issues">Report Bug</a>
    <a href="https://github.com/nhsengland/ESA_Avoidable_ED_Attendances/issues">Request Feature</a>
  </p>
</div>

<!-- table of contents -->
<details>
    <summary>Table of Contents</summary>
    <ol>
        <li>
            <a href="#about-the-project">About the project</a>
            <ul>
                <li><a href="#built-with">Built With</a></li>
            </ul>
        </li>
        <li>
            <a href="#getting-started">Getting Started</a>
            <ul>
                <li><a href="#prerequisites">Prerequisites</a></li>
                <li><a href="#installation">Installation</a></li>
            </ul>
        </li>
        <li><a href="#usage">Usage</a></li>
        <li><a href="#definition">Definition</a></li>
        <li><a href="#performance">Performance</a></li>
        <li><a href="#contributing">Contributing</a></li>
        <li><a href="#license">License</a></li>
        <li><a href="#contact">Contact</a></li>
        <li><a href="#references">References</a></li>
    </ol>
</details>

<!-- About the project -->
## About the Project
The Economics and Strategic Analysis Team, working under the Chief Data and Analytics Officer (CDAO) at NHS England have developed a small package which implements the University of Sheffield's School of Health and Related Research's (ScHARR) definition of 'avoidable' Emergency Department attendances (Mason et al, 2017) with reference to the NHS Digital (2020) adaptation using SNOMED/HES coding. Whilst ECDS superseeds HES, functionality for HES codes is provided for completeness.

### Built With

- R
- C++ (GNU compiler)
- Rcpp (>= 1.0.8)

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- Getting started -->

## Getting Started

### Prerequisites

This package offers the user the choice of using SNOMED or HES codes to derive the metric. We make use of parallelization, where available, enabled through OpenMP, to enable quicker compute when using R data.frames. The majority of the code is written in **C++17**, requiring that and **Rcpp (>=1.0.8)**. Compiled with GNU compiler in testing. By default, the number of threads is set at 50% of those available on the system, however this can be altered via the setESAAvoidableAttThreads() function, or using the nthreads argument. 

### Installation

Our package can be installed directly from GitHub using the **devtools** package.
`devtools::install_github('NHSEngland/ESA_Avoidable_ED_Attendances')`
and can be loaded as with any other R package.
`library(ESAAvoidableAtt)`

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- Usage --->

## Usage

The package returns a logical vector indicating whether each row (attendance) is an 'avoidable' one. The package can be used as follows. Currently, the package only accepts the relevant data columns as being strings rather than integer values. 
`isAvoidable <- calculateAvoidableEDAtt(dt,'Department_Type','Discharge_Status','AttendanceCategory','Arrival_Mode',paste0('Investigation_',1:30),paste0('Treatment_',1:30),'snomed')`

The resultant logical vector can then be bound to the data.frame/data.table using 
`dt <- cbind(dt,isAvoidable)`.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- Definition -->

## Definition

Avoidable (non-urgent) ED attendances here are defined as ED attendances for care that could have been reasonably provided elsewhere. As per NHS Digital (2020), the following HES/SNOMED codes make up the definition.

Emergency Department Type
| Description           | HES code | ECDS code |
|:---------------------:|:--------:|:---------:|
|Type 1 Emergency Dept. | 01       | 01        |

Discharge status (attendance disposal)
| Description                              | HES code |    ECDS (SNOMED) code  |
|:----------------------------------------:|:--------:|:----------------------:|
| Discharged with followup treatment by GP | 02       | 1077021000000100       |
| Discharged no followup required          | 03       | 182992009              |
| Left department before treatment         | 12       | 1066321000000107       |

Investigation 
| Description               | HES code | ECDS (SNOMED) code     |
|:-------------------------:|:--------:|:----------------------:|
| Urinalysis                | 06       | 27171005               |
| Pregnancy test            | 21       | 167252002, 67900009    |
| Dental investigation      | 22       | 53115007               |
| None                      | 24, blank| 1088291000000101, blank|

Treatment 
| Description                                 | HES code  | ECDS (SNOMED) code    |
|:-------------------------------------------:|:---------:|:---------------------:|
| Guidance/advice only - written              | 221       | 413334001             |
| Guidance/advice only - verbal               | 222       | Not applicable        |
| Recording vital signs                       | 30        | Not applicable        |
| Dental treatment                            | 56        | 81733005              |
| Prescription/medicines prepped to take away | 57        | 266712008             |
| None (consider guidance/advice option)      | 99, blank | 183964008, blank      |
| Prescriptions (retired code)                | 07        | Not applicable        |

Attendance category 
| Description                                | HES code | ECDS code |
|:------------------------------------------:|:--------:|:---------:|
| First Accident & Emergency (ED) attendance | 1        | 1         |

Arrival mode 
| Description            | HES code | ECDS (SNOMED) codes                                                 |
|:----------------------:|:--------:|:-------------------------------------------------------------------:|
| Non-ambulance arrivals | Not 1    | 1048071000000103,1048061000000105,1047991000000102,1048001000000106 |

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- performance notes -->

## Performance
On a dataset of 54.5 million rows, using 52 input columns, the following results arose via microbenchmark (small sample of n=10) (measured in seconds).
| threads | n  | min     | lq      | mean    | median  | uq      | max      |
|:-------:|:--:|:-------:|:-------:|:-------:|:-------:|:-------:|:--------:|
| 1       | 10 |841.1406 |851.6867 |932.1591 |924.3097 |987.1517 |1069.6392 |
| 16      | 10 |460.5905 |482.2384 |507.8622 |504.7300 |534.3227 | 551.4016 |
| 30      | 10 |425.0663 |436.6455 |478.5103 |464.3443 |537.4040 | 562.8180 |

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- Contributing -->

## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- LICENSE -->
## License

Unless stated otherwise, the codebase is released under [the MIT License](https://github.com/nhsengland/ESA_ED_Avoidable_Attendances/blob/main/LICENSE). This covers both the codebase and any sample code in the documentation.

_See [LICENSE](https://github.com/nhsengland/ESA_ED_Avoidable_Attendances/blob/main/LICENSE) for more information._

The documentation is [© Crown copyright](http://www.nationalarchives.gov.uk/information-management/re-using-public-sector-information/uk-government-licensing-framework/crown-copyright/) and available under the terms of the [Open Government 3.0](http://www.nationalarchives.gov.uk/doc/open-government-licence/version/3/) license.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- references -->

## References

1. Mason, S., O'Keeffe, C., Jacques, R., Rimmer, M., and Ablard, S., (2017). Perspectives on the reasons for Emergency Department attendances across Yorkshire and the Humber. Centre for Urgent and Emergency Care Research (CURE), School of Health and Related Research, University of Sheffield. Available at https://www.sheffield.ac.uk/polopoly_fs/1.730630!/file/CLAHRC_BMA_Final_Report.pdf
2. NHS Digital, (2020). Non-urgent A&E attendances. Available at https://web.archive.org/web/20210226213236/https:/digital.nhs.uk/data-and-information/data-tools-and-services/data-services/innovative-uses-of-data/demand-on-healthcare/unnecessary-a-and-e-attendances [note page has been archived].

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- Markdown links -->
[issues-shield]: https://img.shields.io/github/issues/nhsengland/ESA_ED_Avoidable_Attendances
[issues-url]: https://github.com/nhsengland/ESA_ED_Avoidable_Attendances/issues
[forks-shield]: https://img.shields.io/github/forks/nhsengland/ESA_ED_Avoidable_Attendances
[forks-url]: https://github.com/nhsengland/ESA_ED_Avoidable_Attendances/network/members
[stars-shield]: https://img.shields.io/github/stars/nhsengland/ESA_ED_Avoidable_Attendances
[stars-url]: https://github.com/nhsengland/ESA_ED_Avoidable_Attendances/stargazers
[license-shield]: https://img.shields.io/github/license/nhsengland/ESA_ED_Avoidable_Attendances
[license-url]: https://github.com/nhsengland/ESA_ED_Avoidable_Attendances/main/blob/LICENSE
