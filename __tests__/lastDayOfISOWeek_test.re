open Jest;

open Js.Date;

describe("lastDayOfISOWeek", () =>
  ExpectJs.(
    test(
      "returns the date with the time set to 00:00:00 and the date set to the last day of an ISO week",
      () => {
        let date =
          makeWithYMDHMS(
            ~year=2014.,
            ~month=8.,
            ~date=2.,
            ~hours=11.,
            ~minutes=55.,
            ~seconds=0.,
            (),
          );
        let expectedDate =
          setHoursMSMs(
            makeWithYMD(~year=2014., ~month=8., ~date=7., ()),
            ~hours=0.,
            ~minutes=0.,
            ~seconds=0.,
            ~milliseconds=0.,
            (),
          )
          |> fromFloat;

        date |> ReDate.lastDayOfISOWeek |> expect |> toEqual(expectedDate);
      },
    )
  )
);