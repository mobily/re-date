open Jest;

open Js.Date;

describe("getISOWeekYear", () => {
  open ExpectJs;

  test("dummy", () => {
    let date = make();

    date |> expect |> not_ |> toEqual(date);
  });

  test("dummy2", () => {
    let date = make();

    date |> expect |> not_ |> toEqual(date);
  });
});