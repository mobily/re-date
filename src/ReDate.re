open Js;

type interval = {
  start: Date.t,
  end_: Date.t,
};

type offset =
  | Start(Date.t)
  | End(Date.t);

[@bs.deriving jsConverter]
type day =
  | Sunday
  | Monday
  | Tuesday
  | Wednesday
  | Thursday
  | Friday
  | Saturday;

type helper = Date.t => Date.t;

type differenceIn =
  | Seconds
  | Hours
  | Minutes
  | Days
  | CalendarDays(helper)
  | Weeks
  | CalendarWeeks(helper)
  | Months
  | CalendarMonths
  | Years
  | CalendarYears;

module Milliseconds = {
  let second = 1000;

  let minute = 60 * second;

  let hour = 60 * minute;

  let day = 24 * hour;

  let week = 7 * day;
};

module Internal = {
  /* based on: https://github.com/date-fns/date-fns/blob/master/src/_lib/getTimezoneOffsetInMilliseconds/index.js */
  let getTimezoneOffsetInMilliseconds = date =>
    date->Date.getTimezoneOffset
    *. Milliseconds.minute->float_of_int
    +. (date->Date.setSecondsMs(~seconds=0., ~milliseconds=0., ())->int_of_float mod Milliseconds.minute)
       ->float_of_int;

  let makeDateWithStartOfDayHours = date =>
    Date.(date->setHoursMSMs(~hours=0., ~minutes=0., ~seconds=0., ~milliseconds=0., ())->fromFloat);

  let makeDateWithEndOfDayHours = date =>
    Date.(date->setHoursMSMs(~hours=23., ~minutes=59., ~seconds=59., ~milliseconds=999., ())->fromFloat);

  let makeLastDayOfMonth = date =>
    Date.(makeWithYMD(~year=date->getFullYear, ~month=date->getMonth +. 1., ~date=0., ()));

  let startOfYear = date =>
    Date.(makeWithYMD(~year=date->getFullYear, ~month=0., ~date=1., ())->makeDateWithStartOfDayHours);

  let getDaysInMonth = date => date->makeLastDayOfMonth->Date.getDate->int_of_float;

  let addMonths = (date, months) =>
    Date.(
      makeWithYMD(
        ~year=date->getFullYear,
        ~month=date->getMonth +. months->float_of_int,
        ~date=Math.min_float(date->getDaysInMonth->float_of_int, date->getDate),
        (),
      )
    );

  let getMillisecondsOf =
    Milliseconds.(
      fun
      | Seconds => second
      | Minutes => minute
      | Hours => hour
      | Days
      | CalendarDays(_) => day
      | Weeks
      | CalendarWeeks(_) => week
      | _ => failwith("error")
    );

  let rec differenceIn = (differenceType, fst, snd) =>
    Date.(
      switch (differenceType) {
      | Seconds
      | Minutes
      | Hours
      | Days
      | Weeks =>
        let diff = (fst->getTime -. snd->getTime) /. differenceType->getMillisecondsOf->float_of_int;
        diff > 0. ? diff->Math.floor_int : diff->Math.ceil_int;
      | Months =>
        let diff = (fst->getMonth -. snd->getMonth +. 12. *. (fst->getFullYear -. snd->getFullYear))->int_of_float;
        let anchor = snd->addMonths(diff);
        let adjust =
          if (fst->getTime -. anchor->getTime < 0.) {
            (fst->getTime -. anchor->getTime) /. (anchor->getTime -. snd->addMonths(diff->pred)->getTime);
          } else {
            (fst->getTime -. anchor->getTime) /. (snd->addMonths(diff->succ)->getTime -. anchor->getTime);
          };

        adjust->Math.round->int_of_float + diff;
      | Years => differenceIn(Months, fst, snd) / 12
      | CalendarDays(startOf)
      | CalendarWeeks(startOf) =>
        let fst = fst->startOf;
        let snd = snd->startOf;
        let fstTime = fst->Date.getTime -. fst->getTimezoneOffsetInMilliseconds;
        let sndTime = snd->Date.getTime -. snd->getTimezoneOffsetInMilliseconds;
        let diff = (fstTime -. sndTime) /. differenceType->getMillisecondsOf->float_of_int;
        diff->Math.round->int_of_float;
      | CalendarMonths =>
        ((fst->getFullYear -. snd->getFullYear) *. 12. +. (fst->getMonth -. snd->getMonth))->int_of_float
      | CalendarYears => (fst->getFullYear -. snd->getFullYear)->int_of_float
      }
    );

  let retrieveMinOrMax = value => value->Belt.Option.getExn->Date.fromFloat;

  let compareAscOrDesc = (tuple, firstDate, secondDate) => {
    let (x, y) = tuple;
    switch (firstDate->Date.getTime -. secondDate->Date.getTime) {
    | ts when ts < 0. => x
    | ts when ts > 0. => y
    | _ => 0
    };
  };

  let reduceMinOrMax = (fn, acc, date) =>
    switch (date->Date.getTime) {
    | ts when acc === None || fn(ts, acc->Belt.Option.getExn) => Some(ts)
    | _ => acc
    };

  let startOrEndOfWeek = (type_, weekStartsOn) => {
    open Date;

    let week = weekStartsOn->dayToJs->float_of_int;

    let date =
      switch (type_) {
      | Start(date) =>
        let day = date->getDay;
        let diff = (day < week ? 7. : 0.) +. day -. week;
        date->setDate(date->getDate -. diff);
      | End(date) =>
        let day = date->getDay;
        let diff = (day < week ? (-7.) : 0.) +. 6. -. (day -. week);
        date->setDate(date->getDate +. diff);
      };

    date->fromFloat;
  };

  let isLeap = year => year mod 400 === 0 || year mod 4 === 0 && year mod 100 !== 0;
};

/* ——[Common helpers]——————————— */

let isEqual = (fst, snd) => Date.(fst->getTime === snd->getTime);

let isAfter = (fst, snd) => Date.(fst->getTime > snd->getTime);

let isBefore = (fst, snd) => Date.(fst->getTime < snd->getTime);

let isFuture = date => date->isAfter(Date.make());

let isPast = date => date->isBefore(Date.make());

let compareAsc = ((-1), 1)->Internal.compareAscOrDesc;

let compareDesc = (1, (-1))->Internal.compareAscOrDesc;

let minOfArray = dates => Internal.(dates->Belt.Array.reduce(None, (<)->reduceMinOrMax)->retrieveMinOrMax);

let minOfList = dates => Internal.(dates->Belt.List.reduce(None, (<)->reduceMinOrMax)->retrieveMinOrMax);

let maxOfArray = dates => Internal.(dates->Belt.Array.reduce(None, (>)->reduceMinOrMax)->retrieveMinOrMax);

let maxOfList = dates => Internal.(dates->Belt.List.reduce(None, (>)->reduceMinOrMax)->retrieveMinOrMax);

/* ——[Second helpers]——————————— */

let addSeconds = (date, seconds) => Date.(date->setSeconds(date->getSeconds +. seconds->float_of_int)->fromFloat);

let subSeconds = (date, seconds) => date->addSeconds(- seconds);

let differenceInSeconds = Internal.differenceIn(Seconds);

let startOfSecond = date => Date.(date->setMilliseconds(0.)->fromFloat);

let endOfSecond = date => Date.(date->setMilliseconds(999.)->fromFloat);

let isSameSecond = (fst, snd) => fst->startOfSecond->isEqual(snd->startOfSecond);

/* ——[Minute helpers]——————————— */

let addMinutes = (date, minutes) => Date.(date->setMinutes(date->getMinutes +. minutes->float_of_int)->fromFloat);

let subMinutes = (date, minutes) => date->addMinutes(- minutes);

let differenceInMinutes = Internal.differenceIn(Minutes);

/* ——[Hour helpers]——————————— */

let addHours = (date, hours) => Date.(date->setHours(date->getHours +. hours->float_of_int)->fromFloat);

let subHours = (date, hours) => date->addHours(- hours);

let differenceInHours = Internal.differenceIn(Hours);

let startOfHour = date => Date.(date->setMinutesSMs(~minutes=0., ~seconds=0., ~milliseconds=0., ())->fromFloat);

let endOfHour = date => Date.(date->setMinutesSMs(~minutes=59., ~seconds=59., ~milliseconds=999., ())->fromFloat);

let isSameHour = (fst, snd) => fst->startOfHour->isEqual(snd->startOfHour);

/* ——[Day helpers]——————————— */

let addDays = (date, days) => Date.(date->setDate(date->getDate +. days->float_of_int)->fromFloat);

let subDays = (date, days) => date->addDays(- days);

let startOfDay = Internal.makeDateWithStartOfDayHours;

let endOfDay = Internal.makeDateWithEndOfDayHours;

let differenceInCalendarDays = Internal.differenceIn(CalendarDays(startOfDay));

let differenceInDays = Internal.differenceIn(Days);

let getDayOfYear = date => date->differenceInCalendarDays(date->Internal.startOfYear)->succ;

let isSameDay = (fst, snd) => fst->startOfDay->isEqual(snd->startOfDay);

let isToday = date => date->isSameDay(Date.make());

let isTomorrow = date => date->isSameDay(Date.make()->addDays(1));

let isYesterday = date => date->isSameDay(Date.make()->subDays(1));

/* ——[Week helpers]——————————— */

let addWeeks = (date, weeks) => date->addDays(weeks * 7);

let subWeeks = (date, weeks) => date->addWeeks(- weeks);

let differenceInWeeks = Internal.differenceIn(Weeks);

let startOfWeek = (~weekStartsOn=Sunday, date) =>
  Internal.(Start(date)->startOrEndOfWeek(weekStartsOn)->makeDateWithStartOfDayHours);

let endOfWeek = (~weekStartsOn=Sunday, date) =>
  Internal.(End(date)->startOrEndOfWeek(weekStartsOn)->makeDateWithEndOfDayHours);

let differenceInCalendarWeeks = (~weekStartsOn=Sunday) => {
  let startOfWeek' = startOfWeek(~weekStartsOn);
  Internal.differenceIn(CalendarWeeks(startOfWeek'));
};

let isSameWeek = (~weekStartsOn=Sunday, fst, snd) => {
  let startOfWeek' = startOfWeek(~weekStartsOn);
  fst->startOfWeek'->isEqual(snd->startOfWeek');
};

let lastDayOfWeek = (~weekStartsOn=Sunday, date) =>
  Internal.(End(date)->startOrEndOfWeek(weekStartsOn)->makeDateWithStartOfDayHours);

/* ——[Weekday helpers]——————————— */

let is = (date, day) => date->Date.getDay === day->dayToJs->float_of_int;

let isSunday = is(_, Sunday);

let isMonday = is(_, Monday);

let isTuesday = is(_, Tuesday);

let isWednesday = is(_, Wednesday);

let isThursday = is(_, Thursday);

let isFriday = is(_, Friday);

let isSaturday = is(_, Saturday);

let isWeekend = date => date->isSaturday || date->isSunday;

/* ——[Month helpers]——————————— */

let getDaysInMonth = Internal.getDaysInMonth;

let addMonths = Internal.addMonths;

let subMonths = (date, months) => date->addMonths(- months);

let differenceInCalendarMonths = Internal.differenceIn(CalendarMonths);

let differenceInMonths = Internal.differenceIn(Months);

let startOfMonth = date => Date.(date->setDate(1.)->fromFloat->Internal.makeDateWithStartOfDayHours);

let endOfMonth = date => Internal.(date->makeLastDayOfMonth->makeDateWithEndOfDayHours);

let isFirstDayOfMonth = date => date->Date.getDate->int_of_float === 1;

let isLastDayOfMonth = date => Date.(date->endOfDay->getTime === date->endOfMonth->getTime);

let isSameMonth = (fst, snd) => fst->startOfMonth->isEqual(snd->startOfMonth);

let lastDayOfMonth = date => Internal.(date->makeLastDayOfMonth->makeDateWithStartOfDayHours);

/* ——[Year helpers]——————————— */

let addYears = (date, years) => date->addMonths(12 * years);

let subYears = (date, years) => date->addYears(- years);

let startOfYear = Internal.startOfYear;

let isSameYear = (fst, snd) => fst->startOfYear->isEqual(snd->startOfYear);

let isLeapYear = date => date->Date.getFullYear->int_of_float->Internal.isLeap;

let endOfYear = date =>
  Date.(makeWithYMD(~year=date->getFullYear, ~month=11., ~date=31., ()))->Internal.makeDateWithEndOfDayHours;

let lastMonthOfYear = date =>
  Date.(makeWithYMD(~year=date->getFullYear, ~month=11., ~date=1., ()))->Internal.makeDateWithStartOfDayHours;

let lastDayOfYear = date => date->lastMonthOfYear->lastDayOfMonth;

let getDaysInYear = date => date->isLeapYear ? 366 : 365;

let differenceInCalendarYears = Internal.differenceIn(CalendarYears);

let differenceInYears = Internal.differenceIn(Years);

/* ——[Interval helpers]——————————— */

let isWithinInterval = (date, ~start, ~end_) => {
  let ts = date->Date.getTime;
  ts >= start->Date.getTime && ts <= end_->Date.getTime;
};

let areIntervalsOverlapping = (left, right) =>
  Date.(left.start->getTime < right.end_->getTime && right.start->getTime < left.end_->getTime);

let getOverlappingDaysInIntervals = (left, right) =>
  Date.(
    switch (left.start->getTime, left.end_->getTime, right.start->getTime, right.end_->getTime) {
    | (lst, let', rst, ret) when lst < ret && rst < let' =>
      let overlapStartTime = rst < lst ? lst : rst;
      let overlapEndTime = ret > let' ? let' : ret;
      let overlap = (overlapEndTime -. overlapStartTime) /. Milliseconds.day->float_of_int;

      overlap->Math.ceil_int;
    | _ => 0
    }
  );

let internal_makeIntervalDay = (interval, index) => interval.start->startOfDay->addDays(index);

let internal_getAmountOfIntervalDays = interval => interval.end_->differenceInCalendarDays(interval.start)->succ;

let eachDayOfIntervalArray = interval =>
  interval->internal_getAmountOfIntervalDays->Belt.Array.makeBy(interval->internal_makeIntervalDay);

let eachDayOfIntervalList = interval =>
  interval->internal_getAmountOfIntervalDays->Belt.List.makeBy(interval->internal_makeIntervalDay);
